#include "apue.h"
#include "apue_db.h"
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/uio.h>

#define IDXLEN_SZ 	4 	//索引记录长度所占字符个数
#define SEP 		':' //索引记录中的分隔符
#define SPACE 		' ' //换行符
#define NEWLINE 	'\n' //换行符

//下面的是针对hash表的一些定义
#define PTR_SZ 		6 	//链表指针占的字符个数
#define PTR_MAX 	999999 //等于10**PTR_SZ - 1, 也就是最大偏移值
#define NHASH_DEF   137    //默认的 hash 表大小，本程序固定hash表的大小，yong137, 因为对于素数 hash 会有比较好的效果
#define FREE_OFF  	0 		//释放链表的表头（所有被释放过的内存块所连起来的链表）
#define HASH_OFF 	PTR_SZ 	//hash 表在索引文件中的偏移位置

typedef unsigned long DBHASH; 	//hash 值
typedef unsigned long COUNT; 	//用来记录的一个类型

//DB 结构体，保存数据库文件的相应信息
typedef struct
{
	int 	idxfd; 		//索引文件描述符
	int 	datfd; 		//数据文件描述符
	char 	*idxbuf; 	//*当前*索引数据的内容, 长度一开始分配好一个最大长度
	char 	*datbuf; 	//*当前*文件数据的内容，长度一开始分配好一个最大长度
	char 	*name; 		//记录数据库文件名，即索引文件和数据文件的共有字符
	off_t 	idxoff; 	//索引记录的偏移位置
	size_t 	idxlen; 	//索引记录的长度
	off_t 	datoff; 	//数据记录的偏移位置
	size_t 	datlen; 	//数据记录的长度

	off_t 	ptrval; 	//这个主要用来表示当前链表中当前节点的前一个节点，方便删除用的
	off_t 	ptroff; 
	off_t 	chainoff; 	//当前链在索引文件中的偏移
	off_t 	hashoff; 	//hash 表在索引文件中的偏移
	DBHASH 	nhash; 		//当前 hash 表的大小，当前程序设置为固定值
	COUNT 	cnt_delok; 	//删除成功的次数
	COUNT 	cnt_delerr; //删除失败的次数
	COUNT 	cnt_fetchok;//取值成功的次数
	COUNT 	cnt_fetcherr;//取值出错的次数
	COUNT 	cnt_nextrec; //求下一个数据项的次数
	COUNT 	cnt_stor1; 	//sotre: 在文件末尾插入一个数据项的次数
	COUNT 	cnt_stor2; 	//store: 从free list重用一个内存单元的次数
	COUNT 	cnt_stor3; 	//store: replace 并且前后两次的长度不一样
	COUNT 	cnt_stor4; 	//store: replace 并且前后两次的长度一样的次数
	COUNT 	cnt_storerr; //store 出错的次数
}DB;

//私有函数
static DB* 		_db_alloc(int);
static void 	_db_dodelete(DB*);
static int 		_db_find_and_lock(DB*, const char*, int);
static int 		_db_findfree(DB*, int, int);
static void 	_db_free(DB*);
static DBHASH 	_db_hash(DB*, const char*);
static char* 	_db_readdat(DB*);
static off_t 	_db_readidx(DB*, off_t);
static off_t 	_db_readptr(DB*, off_t);
static void 	_db_writedat(DB*, const char*, off_t, int);
static void 	_db_writeidx(DB*, const char*, off_t, int, off_t);
static void 	_db_writeptr(DB*, off_t, off_t);

//打开数据库，第一个参数是文件名（不带后缀），第二个是打开参数，后面是不定长参数
DBHANDLE db_open(const char* pathname, int oflag, ...)
{
	DB 		*db;  //数据库句柄
	int 	len, mode; 	//pathname长度 和打开文件的模式
	size_t 	i;
	char 	asciiptr[PTR_SZ + 1], 		//每个指针的值
			hash[(NHASH_DEF + 1) * PTR_SZ + 2]; 	//索引文件中索引记录前面的内存
	struct stat statbuf; 	//文件信息

	len = strlen(pathname);
	if((db = _db_alloc(len)) == NULL) 	//调用_db_alloc 分配具体内存
		err_dump("db_open: _db_alloc error for DB");

	db->nhash = NHASH_DEF; 	//hash 表的大小
	db->hashoff = HASH_OFF; //hash 表开始的偏移位置
	strcpy(db->name, pathname);
	strcat(db->name, ".idx");

	if(oflag & O_CREAT)
	{
		va_list ap;

		va_start(ap, oflag);
		va_end(ap);

		//打开索引文件和数据文件
		db->idxfd = open(db->name, oflag, mode);
		strcpy(db->name + len, ".dat");
		db->datfd = open(db->name, oflag, mode);
	}
	else
	{
		db->idxfd = open(db->name, oflag);
		strcpy(db->name + len, ".dat");
		db->datfd = open(db->name, oflag);
	}

	if(db->idxfd < 0 || db->datfd < 0)
	{ //打开文件出错，释放内存，然后退出
		_db_free(db);
		return NULL;
	}

	if((oflag & (O_CREAT | O_TRUNC)) == (O_CREAT | O_TRUNC))
	{
		//如果新建文件，初始化
		if(writew_lock(db->idxfd, 0, SEEK_SET, 0) < 0) //初始化前加一把写锁
			err_dump("db_open: writew_lock error");

		if(fstat(db->idxfd, &statbuf) < 0) // 获得文件信息
			err_sys("db_open: fstat error");

		if(statbuf.st_size == 0) // 文件长度为0 
		{
			//初始化所有的索引指针为0
			sprintf(asciiptr, "%*d", PTR_SZ, 0);
			hash[0] = 0;
			for(i = 0; i<NHASH_DEF + 1; ++i)
				strcat(hash, asciiptr);
			strcat(hash, "\n");
			i = strlen(hash);
			if(write(db->idxfd, hash, i) != i)
				err_dump("db_open: index file init write error");
		}
		if(un_lock(db->idxfd, 0, SEEK_SET, 0) < 0) // 对文件进行解锁，在_db_find_and_lock里面加锁了
			err_dump("db_opne: un_lock error");
	}
	db_rewind(db);
	return db;
}

static DB* _db_alloc(int namelen) //分配DB结构的内存，namelen代表文件名长度
{
	DB *db;

	//利用calloc， 把所有内存初始化为0
	if((db = calloc(1, sizeof(DB))) == NULL)
		err_dump("_db_alloc: calloc error for DB");
	db->idxfd = db->datfd = -1; // 初始化成-1,代表无效

	//这里的 +5 表示算上后缀(.idx/.dat)然后还有'\0'
	if((db->name = malloc(namelen + 5)) == NULL)
		err_dump("_db_alloc: malloc error for name");
	//+2 表示 '\n' 和 '\0'
	if((db->idxbuf = malloc(IDXLEN_MIN + 2)) == NULL)
		err_dump("_db_alloc: malloc error for idxbuf");
	if((db->datbuf = malloc(DATLEN_MAX + 2)) == NULL)
		err_dump("_db_alloc: malloc error for datbuf");

	return db;
}

//关闭数据库
void db_close(DBHANDLE h)
{
	_db_free((DB*) h); //调用私有函数进行一些内存释放操作
}

static void _db_free(DB* db)
{
	if(db->idxfd >= 0) //如果文件描述符被打开就关闭
		close(db->idxfd);
	if(db->datfd >= 0)
		close(db->datfd);

	//如果分配了其他内存，也需要释放
	if(db->datbuf != NULL)
		free(db->datbuf);
	if(db->idxbuf != NULL)
		free(db->idxbuf);
	if(db->name != NULL)
		free(db->name);

	free(db);
}

//从数据库中取 key 所对应的值
char* db_fetch(DBHANDLE h, const char* key)
{
	DB 		*db = h;
	char 	*ptr;

	if(_db_find_and_lock(db, key, 0) < 0) //如果数据库中存在 key 所对应的值
	{
		ptr = NULL;
		db->cnt_fetcherr++; 	//出错记录
	}
	else
	{
		ptr = _db_readdat(db);   //在_db_find_and_lock 中已经把_db_readdat所需要的参数已经准备好了
		db->cnt_fetchok++;
	}

	if(un_lock(db->idxfd, db->chainoff, SEEK_SET, 1) < 0) //对文件进行解锁
		err_dump("db_fetch: un_lock error");

	return ptr;
}

//在数据库中查找一个项目，找之前进行加锁
//第一个参数代表数据库，第二个是 key，第三个代表加写锁还是读锁
static int
_db_find_and_lock(DB *db, const char *key, int writelock)
{
	off_t	offset, nextoffset;

	//计算 key 所在的链表表头偏移
	db->chainoff = (_db_hash(db, key) * PTR_SZ) + db->hashoff;
	db->ptroff = db->chainoff;

	//对文件进行加锁
	if (writelock) {
		if (writew_lock(db->idxfd, db->chainoff, SEEK_SET, 1) < 0)
			err_dump("_db_find_and_lock: writew_lock error");
	} else {
		if (readw_lock(db->idxfd, db->chainoff, SEEK_SET, 1) < 0)
			err_dump("_db_find_and_lock: readw_lock error");
	}

	//读取当前链表中的第一个记录的偏移
	offset = _db_readptr(db, db->ptroff);
	//在当前链表中查找给定的值
	while (offset != 0) {
		//查找下一个值，并且设置当前值的相应属性
		nextoffset = _db_readidx(db, offset);
		if (strcmp(db->idxbuf, key) == 0)
			break;       /* 找到一个记录*/
		db->ptroff = offset; /* 当前记录的上个记录，删除时有用*/
		offset = nextoffset; /*  更新offset */
	}
	/*
	 * 如果offset 不为0 代表查找成功
	 */
	return(offset == 0 ? -1 : 0);
}
//计算 hash 值
static DBHASH
_db_hash(DB *db, const char *key)
{
	DBHASH		hval = 0;
	char		c;
	int			i;

	for (i = 1; (c = *key++) != 0; i++)
		hval += c * i;		/* ascii char times its 1-based index */
	return(hval % db->nhash);
}
//取offset位置处的链表的第一个取的偏移, 如果链表为空，返回0
static off_t
_db_readptr(DB *db, off_t offset)
{
	char	asciiptr[PTR_SZ + 1];

	if (lseek(db->idxfd, offset, SEEK_SET) == -1)
		err_dump("_db_readptr: lseek error to ptr field");
	if (read(db->idxfd, asciiptr, PTR_SZ) != PTR_SZ)
		err_dump("_db_readptr: read error of ptr field");
	asciiptr[PTR_SZ] = 0;		/* 以0结尾，方便操作*/
	return(atol(asciiptr));
}
//取当前值所在链表的下一个值
static off_t
_db_readidx(DB *db, off_t offset)
{
	ssize_t				i;
	char			*ptr1, *ptr2;
	char			asciiptr[PTR_SZ + 1], asciilen[IDXLEN_SZ + 1];
	struct iovec	iov[2];

	//定位索引文件的位置
	if ((db->idxoff = lseek(db->idxfd, offset,
	  offset == 0 ? SEEK_CUR : SEEK_SET)) == -1)
		err_dump("_db_readidx: lseek error");

	//读取下一个链表指针，以及索引记录长度
	iov[0].iov_base = asciiptr;
	iov[0].iov_len  = PTR_SZ;
	iov[1].iov_base = asciilen;
	iov[1].iov_len  = IDXLEN_SZ;
	if ((i = readv(db->idxfd, &iov[0], 2)) != PTR_SZ + IDXLEN_SZ) {
		if (i == 0 && offset == 0)
			return(-1);		/* EOF for db_nextrec */
		err_dump("_db_readidx: readv error of index record");
	}

	//得到下一个链表指针的数值
	asciiptr[PTR_SZ] = 0;        
	db->ptrval = atol(asciiptr); 

	//跟新索引记录的长度
	asciilen[IDXLEN_SZ] = 0;     
	if ((db->idxlen = atoi(asciilen)) < IDXLEN_MIN ||
	  db->idxlen > IDXLEN_MAX)
		err_dump("_db_readidx: invalid length");

	//读取索引记录，从键开始，到'\n'结束
	if ((i = read(db->idxfd, db->idxbuf, db->idxlen)) != db->idxlen)
		err_dump("_db_readidx: read error of index record");
	if (db->idxbuf[db->idxlen-1] != NEWLINE)   //检查最后一个字符是否是'\n'	
		err_dump("_db_readidx: missing newline");
	db->idxbuf[db->idxlen-1] = 0;	 /* 把'\n'替换成0*/

	//分离出key 数据记录的偏移  数据记录的长度
	//而且db->idxbuf在处理之后只剩键值了
	if ((ptr1 = strchr(db->idxbuf, SEP)) == NULL)
		err_dump("_db_readidx: missing first separator");
	*ptr1++ = 0;				/* replace SEP with null */

	if ((ptr2 = strchr(ptr1, SEP)) == NULL)
		err_dump("_db_readidx: missing second separator");
	*ptr2++ = 0;				/* replace SEP with null */

	if (strchr(ptr2, SEP) != NULL)
		err_dump("_db_readidx: too many separators");

	//更新datoff 也就是数据偏移
	if ((db->datoff = atol(ptr1)) < 0)
		err_dump("_db_readidx: starting offset < 0");
	//更新数据长度
	if ((db->datlen = atol(ptr2)) <= 0 || db->datlen > DATLEN_MAX)
		err_dump("_db_readidx: invalid length");
	//返回下一条记录的偏移值
	return(db->ptrval);		
}
//读取数据文件中的数据
static char *
_db_readdat(DB *db)
{
	//定位文件
	if (lseek(db->datfd, db->datoff, SEEK_SET) == -1)
		err_dump("_db_readdat: lseek error");
	//读取数据内容
	if (read(db->datfd, db->datbuf, db->datlen) != db->datlen)
		err_dump("_db_readdat: read error");
	if (db->datbuf[db->datlen-1] != NEWLINE)	/* 如果数据结尾不是'\n'*/
		err_dump("_db_readdat: missing newline");
	db->datbuf[db->datlen-1] = 0; /* 把'\n'改成0*/
	return(db->datbuf);		/* 返回数据内容*/
}
//删除数据库中指定的数据
int
db_delete(DBHANDLE h, const char *key)
{
	DB		*db = h;
	int		rc = 0;			

	//先在数据库中进行查找, 这里的参数1表示加一把写锁
	if (_db_find_and_lock(db, key, 1) == 0) { //如果查找成功
		//printf("datbuf:::%s::%d\n", db->datbuf,strlen(db->datbuf));
		_db_dodelete(db);  //利用私有函数进行删除
		db->cnt_delok++;
	} else {
		rc = -1;			/* 删除出错*/
		db->cnt_delerr++;
	}
	//对文件进行解锁，在 _db_find_and_lock 中进行加锁
	if (un_lock(db->idxfd, db->chainoff, SEEK_SET, 1) < 0)
		err_dump("db_delete: un_lock error");
	return(rc);
}
//对数据库进行实际的删除操作
static void
_db_dodelete(DB *db)
{
	int		i;
	char	*ptr;
	off_t	freeptr, saveptr;

	//printf("datbuf:::%s\n", db->datbuf);
	//把数据内存和 key 都替换成 SPACE 表示删除
	//这里存在一个疑问，db->datbuf 没有设置? 这里不需要设置，db->datbuf值是一个内存块
	//我们接下来会把这个内存块写到文件中去
	for (ptr = db->datbuf, i = 0; i < db->datlen - 1; i++)
		*ptr++ = SPACE;
	*ptr = 0;	
	ptr = db->idxbuf;
	while (*ptr)
		*ptr++ = SPACE;

	//删除的时候会更改free list，所以对其进行加锁
	if (writew_lock(db->idxfd, FREE_OFF, SEEK_SET, 1) < 0)
		err_dump("_db_dodelete: writew_lock error");

	//把更改写到文件中
	_db_writedat(db, db->datbuf, db->datoff, SEEK_SET);

	//得到free list的第一条数据偏移位置
	freeptr = _db_readptr(db, FREE_OFF);

	//db->ptrval 表示删除节点的前一个节点，这个在 _db_find_and_lock里面设置
	saveptr = db->ptrval;

	//更改索引记录到文件，表示删除
	_db_writeidx(db, db->idxbuf, db->idxoff, SEEK_SET, freeptr);

	//把删除的内存段添加到 free list 中
	_db_writeptr(db, FREE_OFF, db->idxoff);

	//更新 free list 的表头
	_db_writeptr(db, db->ptroff, saveptr);
	//对加锁文件进行解锁
	if (un_lock(db->idxfd, FREE_OFF, SEEK_SET, 1) < 0)
		err_dump("_db_dodelete: un_lock error");
}
//更新数据文件
//第一个参数表示数据库，第二个参数表示要写的数据
//第三个参数表示偏移，第四个参数表示从哪开始
static void
_db_writedat(DB *db, const char *data, off_t offset, int whence)
{
	struct iovec	iov[2];
	static char		newline = NEWLINE;

	//如果写到文件末尾的话，也就是添加新记录，需要对文件加锁
	if (whence == SEEK_END) 
		if (writew_lock(db->datfd, 0, SEEK_SET, 0) < 0)
			err_dump("_db_writedat: writew_lock error");

	//设置偏移量
	if ((db->datoff = lseek(db->datfd, offset, whence)) == -1)
		err_dump("_db_writedat: lseek error");
	db->datlen = strlen(data) + 1; // +1 表示'\n'	

	iov[0].iov_base = (char *) data;
	iov[0].iov_len  = db->datlen - 1;
	iov[1].iov_base = &newline;
	iov[1].iov_len  = 1;
	//把data和'\n'写入文件，这里没有直接把'\n'加到data的末尾
	//是因为可能data后面没有足够的空间
	if (writev(db->datfd, &iov[0], 2) != db->datlen)
		err_dump("_db_writedat: writev error of data record");

	//如果前面进行了加锁，在这里需要进行解锁
	if (whence == SEEK_END)
		if (un_lock(db->datfd, 0, SEEK_SET, 0) < 0)
			err_dump("_db_writedat: un_lock error");
}
//更新索引文件
//第一个参数代表数据库，第二个参数是键值
//第三个参数是偏移量，第四个参数代表从哪开始偏移
//第五个参数是插入前这条链的表头偏移值（0表示没有）
static void
_db_writeidx(DB *db, const char *key,
             off_t offset, int whence, off_t ptrval)
{
	struct iovec	iov[2];
	char			asciiptrlen[PTR_SZ + IDXLEN_SZ +1];
	int				len;
	char			*fmt;

	if ((db->ptrval = ptrval) < 0 || ptrval > PTR_MAX)
		err_quit("_db_writeidx: invalid ptr: %d", ptrval);
	if (sizeof(off_t) == sizeof(long long))
		fmt = "%s%c%lld%c%d\n";
	else
		fmt = "%s%c%ld%c%d\n";
	//格式化 idxbuf 内容
	sprintf(db->idxbuf, fmt, key, SEP, db->datoff, SEP, db->datlen);
	if ((len = strlen(db->idxbuf)) < IDXLEN_MIN || len > IDXLEN_MAX)
		err_dump("_db_writeidx: invalid length");
	//格式化偏移值
	sprintf(asciiptrlen, "%*ld%*d", PTR_SZ, ptrval, IDXLEN_SZ, len);

	//如果我们是在追加一条记录的话，那么就需要对文件加锁
	if (whence == SEEK_END)		/* we're appending */
		if (writew_lock(db->idxfd, ((db->nhash+1)*PTR_SZ)+1,
		  SEEK_SET, 0) < 0)
			err_dump("_db_writeidx: writew_lock error");

	//设置偏移值
	if ((db->idxoff = lseek(db->idxfd, offset, whence)) == -1)
		err_dump("_db_writeidx: lseek error");

	iov[0].iov_base = asciiptrlen;
	iov[0].iov_len  = PTR_SZ + IDXLEN_SZ;
	iov[1].iov_base = db->idxbuf;
	iov[1].iov_len  = len;
	//把索引记录写道文件中
	if (writev(db->idxfd, &iov[0], 2) != PTR_SZ + IDXLEN_SZ + len)
		err_dump("_db_writeidx: writev error of index record");

	//如果上面进行了加锁，在这里进行解锁
	if (whence == SEEK_END)
		if (un_lock(db->idxfd, ((db->nhash+1)*PTR_SZ)+1,
		  SEEK_SET, 0) < 0)
			err_dump("_db_writeidx: un_lock error");
}
//更新一个链表指针
//第一个参数代表数据库，第二个参数代表偏移量
//第三个参数代表需要写的值
static void
_db_writeptr(DB *db, off_t offset, off_t ptrval)
{
	char	asciiptr[PTR_SZ + 1];

	if (ptrval < 0 || ptrval > PTR_MAX)
		err_quit("_db_writeptr: invalid ptr: %d", ptrval);
	//格式化链表指针（都占 PTR_SZ 个字符）
	sprintf(asciiptr, "%*ld", PTR_SZ, ptrval);

	//定位并进行数据的更改
	if (lseek(db->idxfd, offset, SEEK_SET) == -1)
		err_dump("_db_writeptr: lseek error to ptr field");
	if (write(db->idxfd, asciiptr, PTR_SZ) != PTR_SZ)
		err_dump("_db_writeptr: write error of ptr field");
}
//对数据操作的主要函数
//第一个参数代表数据库，第二个是键值
//第三个是数据值，第三个代表操作类型
int
db_store(DBHANDLE h, const char *key, const char *data, int flag)
{
	DB		*db = h;
	int		rc, keylen, datlen;
	off_t	ptrval;

	//如果操作类型不对，返回出错信息
	if (flag != DB_INSERT && flag != DB_REPLACE &&
	  flag != DB_STORE) {
		errno = EINVAL;
		return(-1);
	}
	//得到键值和数据值的长度
	keylen = strlen(key);
	datlen = strlen(data) + 1;		/* +1 表示后面的 '\n'*/
	if (datlen < DATLEN_MIN || datlen > DATLEN_MAX)
		err_dump("db_store: invalid data length");

	//printf("Datlen:%d:%s:%s\n", datlen,key, data);
	//在数据中进行查找，是否存在键值相等的项
	if (_db_find_and_lock(db, key, 1) < 0) { //不存在
		if (flag == DB_REPLACE) { //如果操作是替换的话，出错
			rc = -1;
			db->cnt_storerr++;
			errno = ENOENT;	
			goto doreturn;
		}

		//得到 key 所在的链表的第一个项的偏移值
		//db->chainoff 在 _db_find_and_lock 中进行设置
		ptrval = _db_readptr(db, db->chainoff);

		//在 free list 中进行查找，是否有合适的位置可以重用
		if (_db_findfree(db, keylen, datlen) < 0) {
			//没有合适的位置，在文件末尾进行添加
			_db_writedat(db, data, 0, SEEK_END); //更性db->datoff和db->datlen
			_db_writeidx(db, key, 0, SEEK_END, ptrval);//更新db->idxoff

			//这里更新链表头
			_db_writeptr(db, db->chainoff, db->idxoff);
			db->cnt_stor1++;
		} else {
			//重用 free list 中的项，直接修改内容即可，在 _db_findfree中已经把项从 free list 中删除掉了
			_db_writedat(db, data, db->datoff, SEEK_SET);
			_db_writeidx(db, key, db->idxoff, SEEK_SET, ptrval);
			_db_writeptr(db, db->chainoff, db->idxoff);
			db->cnt_stor2++;
		}
	} else {					//该项存在于数据库中	
		if (flag == DB_INSERT) { //如果进行插入操作，出错
			rc = 1;		
			db->cnt_storerr++;
			goto doreturn;
		}

		//printf("datlen:%d\n", datlen);
		//下面是对数据进行 REPLACE
		//考虑数据长度是否一样，一样直接修改数据即可
		if (datlen != db->datlen) {  //不一样
			//printf("IN:%d:%d:%d\n", datlen, keylen, db->datlen);
			_db_dodelete(db);	/* 删除当前项*/

			//读取链表的表头
			ptrval = _db_readptr(db, db->chainoff);

			//在文件末尾添加一项数据
			_db_writedat(db, data, 0, SEEK_END);
			_db_writeidx(db, key, 0, SEEK_END, ptrval);

			//更改链表表头，添加到表头位置
			_db_writeptr(db, db->chainoff, db->idxoff);
			db->cnt_stor3++;
		} else {
			//数据长度一样，直接修改数据即可
			_db_writedat(db, data, db->datoff, SEEK_SET);
			db->cnt_stor4++;
		}
	}
	rc = 0;		

doreturn:
	//对文件进行解锁，然后返回
	if (un_lock(db->idxfd, db->chainoff, SEEK_SET, 1) < 0)
		err_dump("db_store: un_lock error");
	return(rc);
}
//从 free list 中查找合适的项
//第一个参数是数据库，第二个是键的长度，第三个是数据长度
static int
_db_findfree(DB *db, int keylen, int datlen)
{
	int		rc;
	off_t	offset, nextoffset, saveoffset;

	//因为可能需要从 free list 中删除，所以先加锁
	if (writew_lock(db->idxfd, FREE_OFF, SEEK_SET, 1) < 0)
		err_dump("_db_findfree: writew_lock error");

	//得到 free list 的表头偏移位置
	saveoffset = FREE_OFF;
	offset = _db_readptr(db, saveoffset);

	//printf("offset:%d:datlen:%d\n", offset,datlen);
	//对整个 free list 进行查找
	while (offset != 0) {
		nextoffset = _db_readidx(db, offset); //读取下一条记录的偏移，并设置当前记录的相应信息
		if (strlen(db->idxbuf) == keylen && db->datlen == datlen)
			break;		/* 找到了一条合适的数据*/
		saveoffset = offset;
		offset = nextoffset;
	}

	if (offset == 0) {
		rc = -1;	/* 没有找到数据*/
	} else {
		//找到了合适的，把当前项删除，saveoffset 表示当前的偏移
		//db->ptrval 在 _db_readidx 中设置为上一条记录的偏移
		//这条语句就跳过了当前节点，相当于删除
		_db_writeptr(db, saveoffset, db->ptrval);
		rc = 0;

	}

	//进行解锁
	if (un_lock(db->idxfd, FREE_OFF, SEEK_SET, 1) < 0)
		err_dump("_db_findfree: un_lock error");

	//printf("JKL:%d:%d:%d:%d\n",datlen, keylen, db->datlen, rc);
	return(rc);
}
//把索引文件的偏移设置到开始处
void
db_rewind(DBHANDLE h)
{
	DB		*db = h;
	off_t	offset;

	//计算索引记录的开始位置 +1 表示 free list
	offset = (db->nhash + 1) * PTR_SZ;	

	//修改文件偏移
	if ((db->idxoff = lseek(db->idxfd, offset+1, SEEK_SET)) == -1)
		err_dump("db_rewind: lseek error");
}

//返回下一条记录
//第一个参数表示数据库，第二个参数如果不是 NULL 的话，把键值复制给它
//调用者保证有足够的空间
char *
db_nextrec(DBHANDLE h, char *key)
{
	DB		*db = h;
	char	c;
	char	*ptr;

	//锁住 free list 防止我们读取的时候，有人在删除
	if (readw_lock(db->idxfd, FREE_OFF, SEEK_SET, 1) < 0)
		err_dump("db_nextrec: readw_lock error");

	do {
		//读取下一条索引记录
		if (_db_readidx(db, 0) < 0) {
			ptr = NULL;		/* 有错或者到达文件末尾*/
			goto doreturn;
		}

		//检查键值是否是 SPACE ，SPACE 代表删除掉的索引
		ptr = db->idxbuf;
		while ((c = *ptr++) != 0  &&  c == SPACE)
			;	
	} while (c == 0);	

	//复制键值
	if (key != NULL)
		strcpy(key, db->idxbuf);	
	ptr = _db_readdat(db); //返回数据值	
	db->cnt_nextrec++;

doreturn:
	if (un_lock(db->idxfd, FREE_OFF, SEEK_SET, 1) < 0)
		err_dump("db_nextrec: un_lock error");
	return(ptr);
}
int main(void)
{
	DBHANDLE 	db;

	if((db = db_open("db4", O_RDWR | O_CREAT | O_TRUNC, FILE_MODE)) == NULL)
		err_sys("db_open error");

	if(db_store(db, "Alpha", "data1", DB_INSERT) != 0)
		err_quit("db_store error for alpha");
	if(db_store(db, "beta", "data for beta", DB_INSERT) != 0)
		err_quit("db_store error for beta");
	if(db_store(db, "gamma", "record3", DB_INSERT) != 0)
		err_quit("db_store error for gamma");

	db_close(db);
	return 0;
}
