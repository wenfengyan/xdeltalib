/*
* Copyright (C) 2013- yeyouqun@163.com
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, visit the http://fsf.org website.
*/
#ifndef __XDELTA_LIB_H__
#define __XDELTA_LIB_H__

/// \namespace xdelta
/// \brief 最外层名字空间 xdelta
/// xdeltalib 所有的实现均在此名字空间中。
namespace xdelta {

/// \struct
/// 用来描述目标文件的位置信息
struct target_pos
{
	uint64_t	t_offset;		///< Hash 值在目标文件的文件，index 基于这个值来表示。
								///< 本参数只在多轮 Hash 时才会用到，其他情况下为 0。
	uint32_t	index;          ///< Hash 数据块的块索引值。
};

/// \struct
/// 用来描述慢 Hash 值。
struct slow_hash {
    uchar_t		hash[DIGEST_BYTES]; ///< 数据块的 MD4 Hash 值。
	target_pos	tpos;				///< 数据块在目标文件中的位置信息。
};

/// \struct
/// 在多轮 Hash 过程中用来描述文件洞，即 Hash 所处理的文件区域
struct hole_t
{
	uint64_t offset;		///< 文件的偏移。
	uint64_t length;		///< 文件洞的长度。
	hole_t () : offset (0), length (0) {}
};

} // namespace xdelta

NAMESPACE_STD_BEGIN

template <> struct less<xdelta::hole_t> {
	bool operator () (const xdelta::hole_t & left, const xdelta::hole_t & right) const
	{
		return right.offset + right.length < left.offset;
	}
};

template <> struct less<xdelta::slow_hash> {
	bool operator () (const xdelta::slow_hash & left, const xdelta::slow_hash & right) const
	{
		return memcmp (left.hash, right.hash, DIGEST_BYTES) < 0 ? true : false;
	}
};

NAMESPACE_STD_END

namespace xdelta {

class DLL_EXPORT xdelta_stream 
{
public:
	virtual ~xdelta_stream () {} 
	/// \brief
	/// 输出一个相同块的块信息记录
	/// \param[in] tpos		块在目标文件中的位置信息。
	/// \param[in] blk_len	块长度。
	/// \param[in] s_offset	相同块在源文件中的位置偏移。
	/// \return 没有返回
	virtual void add_block (const target_pos & tpos
							, const uint32_t blk_len
							, const uint64_t s_offset) { THROW_XDELTA_EXCEPTION ("Not implemented.!"); }
	/// \brief
	/// 输出一个差异的块数据到流中。
	/// \param[in] data		差异数据块指针。
	/// \param[in] blk_len	数据块长度。
	/// \param[in] s_offset	数据块在源文件中的位置偏移。
	/// \return 没有返回
	virtual void add_block (const uchar_t * data
							, const uint32_t blk_len
							, const uint64_t s_offset) { THROW_XDELTA_EXCEPTION ("Not implemented.!"); }

};

class DLL_EXPORT hasher_stream 
{
public:
	virtual ~hasher_stream () {}
	/// \brief
	/// 输出一个块数据的快、慢 Hash 值。
	/// \param[in] fhash		快 Hash 值。
	/// \param[in] shash		慢 Hash 值。
	virtual void add_block (const uint32_t fhash, const slow_hash & shash)
	 { THROW_XDELTA_EXCEPTION ("Not implemented.!"); }
};

/// 在单轮 Hash 中的最小块长度
#define XDELTA_BLOCK_SIZE 400

/// 在单轮 Hash 中的最大块长度
#define MAX_XDELTA_BLOCK_BYTES ((xdelta::int32_t)1 << 20) // 1024KB

/// 在一些内存受限系统中，如果下面的参数定义得很多，有可能导致内存耗用过多，使系统
/// 运行受到影响，甚至是宕机，所以，当你需要你的目标系统的内存特性后，请你自己定义相应的
/// 的大小。不过建议 MULTIROUND_MAX_BLOCK_SIZE 不要小于 512 KB，XDELTA_BUFFER_LEN 必须大于
/// MULTIROUND_MAX_BLOCK_SIZE，最好为 2 的 N 次方倍，如 8 倍 等。
/// 当你的系统中存在大量内存时，较大的内存可以优化实现。使用库同步数据时，软件系统最多占用内存
/// 的大概为：
///		在数据源端（客户端）：
///			线程数 * XDELTA_BUFFER_LEN * 3，如果系统内存受限，你可以采用少线程的方式进行处理方式，
///			但是你无法使用多线程的优势。
///		在数据目标端（服务端）：
///			线程数 * XDELTA_BUFFER_LEN * 2，但是线程数量受到并发的客户端的数目以及每客户端在同步数据时
///			采用的线程数。
/// 由于在同步时，如果文件大小或者块大小，没有达到 XDELTA_BUFFER_LEN 长度，则未被使用的地址系统不会分配
/// 物理内存，因此有时只会占用进程的地址空间，但却不会占用系统的物理内存。
#ifndef BIT32_PLATFORM
#error "Define BIT32_PLATFORM first, maybe you have to include mytypes.h first!"
#endif
#if BIT32_PLATFORM
	/// 在多轮 Hash 中的最大块长度
	#define MULTIROUND_MAX_BLOCK_SIZE (1 << 22)

	/// 库中使用缓存长度
	#define XDELTA_BUFFER_LEN ((int32_t)1 << 25) // 32MB
#else
	/// 在多轮 Hash 中的最大块长度
	#define MULTIROUND_MAX_BLOCK_SIZE (1 << 20)

	/// 库中使用缓存长度
	#define XDELTA_BUFFER_LEN ((int32_t)1 << 23) // 8MB
#endif

#define MAX(a,b) ((a)>(b)?(a):(b))
    
/// \fn int32_t minimal_multiround_block
/// \brief 取得在多轮 Hash 中最小的块长度, refer to:<<Multiround Rsync.pdf>> to get
/// more details for multiround rsync.
inline int32_t minimal_multiround_block ()
{
	return XDELTA_BLOCK_SIZE;
}

/// \fn int32_t multiround_base 
/// \brief
/// 返回多轮 Hash 中的块基数，即块大小每次除以这个基数，refer to:<<Multiround Rsync.pdf>> to get
/// more details for multiround rsync.
inline int32_t multiround_base ()
{
#define MULTIROUND_BASE_VALUE 3
	return MULTIROUND_BASE_VALUE;
}

class DLL_EXPORT hash_table  {
#ifdef _WIN32
	hash_map<uint32_t, std::set<slow_hash> * > hash_table_;
	typedef hash_map<uint32_t, std::set<slow_hash> *>::const_iterator chit_t;
	typedef hash_map<uint32_t, std::set<slow_hash> *>::iterator hash_iter;
#else
	__gnu_cxx::hash_map<uint32_t, std::set<slow_hash> * > hash_table_;
	typedef __gnu_cxx::hash_map<uint32_t, std::set<slow_hash> *>::const_iterator chit_t;
	typedef __gnu_cxx::hash_map<uint32_t, std::set<slow_hash> *>::iterator hash_iter;
#endif
public:
	hash_table () {}
	virtual ~hash_table ();
	/// \brief
	/// 清除所有的 Hash 值。
	/// \return		no return.
	///
	void clear ();
	/// \brief
	/// 检查 Hash 表是否为空。
	/// \return If hash table is empty,return true, otherwise return false
	///
	bool empty () const { return hash_table_.empty (); }
	/// \brief
	/// 在 Hash 表中插入一个快慢 Hash 对。
	/// \param[in]	fhash 慢速 Hash 值。
	/// \param[in]	shash 慢速 Hash 值。
	/// \return		no return.
	///
	void add_block (const uint32_t fhash, const slow_hash & shash);

	/// \brief
	/// 在表中查找是否有指定块的 Hash 对。
	/// \param[in] fhash	慢速 Hash 值。
	/// \param[in] buf		数据块指针。
	/// \param[in] len		数据块长度
	/// \return	如果存在与指定块相同的 Hash 对，则返回这个 Hash 对的指针，否则返回 0.
	const slow_hash * find_block (const uint32_t fhash
									, const uchar_t * buf
									, const uint32_t len) const;
	/// \brief
	/// 生成文件的 Hash 对，并流化输出。
	/// \param[in] reader	文件对象，数据从这个文件对像中读取数据。
	/// \param[in] stream	流化对象。
	/// \return	No return
	virtual void hash_it (file_reader & reader, hasher_stream & stream) const;
};


/// \fn uint32_t DLL_EXPORT get_xdelta_block_size (const uint64_t filesize)
/// \brief 根据文件大小计算相应的 Hash 块长度。
/// \param[in] filesize 文件的大小。
/// \return     对应的块长度。
uint32_t DLL_EXPORT get_xdelta_block_size (const uint64_t filesize);

/// \class
/// 用于计算快 Hash 值。
class DLL_EXPORT rolling_hasher
{
public:
	rolling_hasher () { RollsumInit ((&sum_)); }
	/// \brief
	/// 计算输出参数的快 hash 值。
	/// \param buf1[in] 数据块指针。
	/// \param len[in]  数据块的长度。
	/// \return     数据块的快 Hash 值。
    static uint32_t hash(const uchar_t *buf1, uint32_t len) {
		Rollsum sum;
		RollsumInit ((&sum))
    	RollsumUpdate (&sum, buf1, len);
		return RollsumDigest ((&sum));
    }
	
	/// \brief
	/// 用输入的数据块初始化对角内部状态，准备调用 Rolling Hash 接口 update。
	/// \param buf1[in] 数据块指针。
	/// \param len[in]  数据块的长度。
	/// \return     没有返回。
	void eat_hash (const uchar_t *buf1, uint32_t len)
	{
		RollsumInit ((&sum_));
#ifdef _WIN32
		std::for_each (buf1, buf1 + len, std::bind1st (std::mem_fun (&rolling_hasher::_eat), this));
#else
		std::for_each (buf1, buf1 + len
			, std::bind1st (__gnu_cxx::mem_fun1 (&rolling_hasher::_eat), this));
#endif
	}
	
	/// \brief
	/// 返回对像当前的快 Hash 值。
	/// \return     返回对像当前的快 Hash 值。
	uint32_t hash_value () const { return RollsumDigest ((&sum_)); }
    
	/// \brief
	/// 通过出与入字节，执行 Rolling Hash。
	/// \param[in] outchar 窗口中滑出的字节。
	/// \param[in] inchar  窗口中滑入的字节。
	/// \return     返回对像当前的快 Hash 值。
    uint32_t update(uchar_t outchar, uchar_t inchar) {
		RollsumRotate (&sum_, outchar, inchar);
		return RollsumDigest ((&sum_));
    } 
private:
	Rollsum sum_;
	void _eat (uchar_t inchar) { RollsumRollin ((&sum_), inchar); }
};

/// \fn void get_file_digest (file_reader & reader, uchar_t digest[DIGEST_BYTES])
/// \brief 取得文件的 MD4 Hash 值。
/// \param[in] reader	    文件读取对像。
/// \param[in] reader	    Hash 值输出地址。
/// \return     无返回
void DLL_EXPORT get_file_digest (file_reader & reader, uchar_t digest[DIGEST_BYTES]);

/// \struct
/// 用于释放用户传入库中的对象。
struct DLL_EXPORT deletor
{
	virtual void release (file_reader * p) = 0;
	virtual void release (hasher_stream * p) = 0;
	virtual void release (xdelta_stream * p) = 0;
	virtual void release (hash_table * p) = 0;
};

/// \fn char_buffer<char_type> & operator << (char_buffer<char_type> & buff, const slow_hash & var)
/// \brief 将 slow_hash 对象流化到 buff 中。
/// \param[in] buff char_buff 对象。
/// \param[in] var  Slow Hash 对象。
/// \return buff char_buff 的引用。
template <typename char_type>
inline char_buffer<char_type> & operator << (char_buffer<char_type> & buff, const slow_hash & var)
{
	buff << var.tpos.index << var.tpos.t_offset;
	buff.copy (var.hash, DIGEST_BYTES);
	return buff;
}

/// \fn char_buffer<char_type> & operator >> (char_buffer<char_type> & buff, slow_hash & var)
/// \brief 从 buff 中反流化一个 slow_hash 对象。
/// \param[in] buff char_buff 对象。
/// \param[out] var  Slow Hash 对象。
/// \return buff char_buff 的引用。
template <typename char_type>
inline char_buffer<char_type> &	operator >> (char_buffer<char_type> & buff, slow_hash & var)
{
	buff >> var.tpos.index >> var.tpos.t_offset;
	memcpy (var.hash, buff.rd_ptr (), DIGEST_BYTES);
	buff.rd_ptr (DIGEST_BYTES);
	return buff;
}

/// \fn bool is_no_file_error (int32_t error_no)
/// \brief 判断错误代码是否是文件不存在的错误，如目录不存在，文件不存在则返回真，否则返回假。
/// \param[in] error_no 错误代码。
/// \return 如目录不存在，文件不存在则返回真，否则返回假。
inline bool is_no_file_error (const int32_t error_no)
{
	if (error_no != 0 && error_no != ENOENT
#ifdef _WIN32
			&& error_no != ERROR_FILE_NOT_FOUND
			&& error_no != ERROR_PATH_NOT_FOUND
#endif
			) {
		return false;
	}
	return true;
}

/// 版本宏，在通信时，通过 BT_CLIENT_BLOCK 最开始的两个字节，版本策略
/// 是向后兼容，并且每次更新增加 1。在可正常接收信息时，向客户端发送版本信息，以及错误信息。
#ifdef _WIN32
	#define XDELTA_VERSION (1)
#else
	#define XDELTA_VERSION ((short)1)
#endif
/// 版本不匹配
#define ERR_DISCOMPAT_VERSION (-1)
#define ERR_UNKNOWN_VERSION (-2)
#define	ERR_INCORRECT_BLOCK_TYPE (-3)

struct handshake_header
{
	handshake_header() : version(XDELTA_VERSION)
	, error_no(0)
	{
		memset(reserved, 0, 32);
	}
	void init()
	{
		version = XDELTA_VERSION;
		error_no = 0;
		memset(reserved, 0, 32);
	}
	int16_t		version;
	int32_t		error_no;
	uchar_t		reserved[32];
};

/// \fn char_buffer<char_type> & operator << (char_buffer<char_type> & buff, const handshake_header & var)
/// \brief 将对象变量流化到 buff 中。
/// \param[in] buff char_buff 对象。
/// \param[in] var  变量值。
/// \return buff char_buff 的引用。
template <typename char_type>
inline char_buffer<char_type> & operator << (char_buffer<char_type> & buff, const handshake_header & var)
{
	buff << var.version << var.error_no;
	buff.copy(var.reserved, 32);
	return buff;
}

/// \fn char_buffer<char_type> & operator >> (char_buffer<char_type> & buff, handshake_header & var)
/// \brief 将对象变量反流化到 buff 中。
/// \param[in] buff char_buff 对象。
/// \param[in] var  变量值。
/// \return buff char_buff 的引用。
template <typename char_type>
inline char_buffer<char_type> & operator >> (char_buffer<char_type> & buff, handshake_header & var)
{
	buff >> var.version >> var.error_no;
	memcpy(var.reserved, buff.rd_ptr(), 32);
	buff.rd_ptr(32);
	return buff;
}

#define BEGINE_HEADER(buff)	do {		\
		buff.reset();					\
		buff.wr_ptr(BLOCK_HEAD_LEN);	\
	} while (0)

#define END_HEADER(buff,type)	do {							\
		block_header header;									\
		header.blk_type = type;									\
		header.blk_len = (uint32_t)(((buff).wr_ptr()			\
				- (buff).begin()) - BLOCK_HEAD_LEN);			\
		char_buffer<uchar_t> tmp(buff.begin(), STACK_BUFF_LEN); \
		tmp << header;											\
	}while (0)
	
void read_and_hash (file_reader & reader
							, hasher_stream & stream
							, uint64_t to_read_bytes
							, const int32_t blk_len
							, uint64_t t_offset
							, rs_mdfour_t * pctx);
							
void read_and_delta (file_reader & reader
					, xdelta_stream & stream
					, const hash_table & hashes
					, std::set<hole_t> & hole_set
					, const int blk_len
					, bool need_split_hole);
} // namespace xdelta
#endif /*__XDELTA_LIB_H__*/

