#include "opencv2/core/types_c.h"

#include <set>
#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <deque>


typedef struct CvFileStorage CvFileStorage;

/** Storage flags: */
#define CV_STORAGE_READ          0
#define CV_STORAGE_WRITE         1
#define CV_STORAGE_WRITE_TEXT    CV_STORAGE_WRITE
#define CV_STORAGE_WRITE_BINARY  CV_STORAGE_WRITE
#define CV_STORAGE_APPEND        2
#define CV_STORAGE_MEMORY        4
#define CV_STORAGE_FORMAT_MASK   (7<<3)
#define CV_STORAGE_FORMAT_AUTO   0
#define CV_STORAGE_FORMAT_XML    8
#define CV_STORAGE_FORMAT_YAML  16
#define CV_STORAGE_FORMAT_JSON  24
#define CV_STORAGE_BASE64       64
#define CV_STORAGE_WRITE_BASE64  (CV_STORAGE_BASE64 | CV_STORAGE_WRITE)

/** @brief List of attributes. :

In the current implementation, attributes are used to pass extra parameters when writing user
objects (see cvWrite). XML attributes inside tags are not supported, aside from the object type
specification (type_id attribute).
@see cvAttrList, cvAttrValue
 */
typedef struct CvAttrList
{
    const char** attr;         /**< NULL-terminated array of (attribute_name,attribute_value) pairs. */
    struct CvAttrList* next;   /**< Pointer to next chunk of the attributes list.                    */
}
CvAttrList;

/** initializes CvAttrList structure */
CV_INLINE CvAttrList cvAttrList( const char** attr CV_DEFAULT(NULL),
                                 CvAttrList* next CV_DEFAULT(NULL) )
{
    CvAttrList l;
    l.attr = attr;
    l.next = next;

    return l;
}

struct CvTypeInfo;

#define CV_NODE_NONE        0
#define CV_NODE_INT         1
#define CV_NODE_INTEGER     CV_NODE_INT
#define CV_NODE_REAL        2
#define CV_NODE_FLOAT       CV_NODE_REAL
#define CV_NODE_STR         3
#define CV_NODE_STRING      CV_NODE_STR
#define CV_NODE_REF         4 /**< not used */
#define CV_NODE_SEQ         5
#define CV_NODE_MAP         6
#define CV_NODE_TYPE_MASK   7

#define CV_NODE_TYPE(flags)  ((flags) & CV_NODE_TYPE_MASK)

/** file node flags */
#define CV_NODE_FLOW        8 /**<Used only for writing structures in YAML format. */
#define CV_NODE_USER        16
#define CV_NODE_EMPTY       32
#define CV_NODE_NAMED       64

#define CV_NODE_IS_INT(flags)        (CV_NODE_TYPE(flags) == CV_NODE_INT)
#define CV_NODE_IS_REAL(flags)       (CV_NODE_TYPE(flags) == CV_NODE_REAL)
#define CV_NODE_IS_STRING(flags)     (CV_NODE_TYPE(flags) == CV_NODE_STRING)
#define CV_NODE_IS_SEQ(flags)        (CV_NODE_TYPE(flags) == CV_NODE_SEQ)
#define CV_NODE_IS_MAP(flags)        (CV_NODE_TYPE(flags) == CV_NODE_MAP)
#define CV_NODE_IS_COLLECTION(flags) (CV_NODE_TYPE(flags) >= CV_NODE_SEQ)
#define CV_NODE_IS_FLOW(flags)       (((flags) & CV_NODE_FLOW) != 0)
#define CV_NODE_IS_EMPTY(flags)      (((flags) & CV_NODE_EMPTY) != 0)
#define CV_NODE_IS_USER(flags)       (((flags) & CV_NODE_USER) != 0)
#define CV_NODE_HAS_NAME(flags)      (((flags) & CV_NODE_NAMED) != 0)

#define CV_NODE_SEQ_SIMPLE 256
#define CV_NODE_SEQ_IS_SIMPLE(seq) (((seq)->flags & CV_NODE_SEQ_SIMPLE) != 0)

typedef struct CvString
{
    int len;
    char* ptr;
}
CvString;

/** All the keys (names) of elements in the readed file storage
   are stored in the hash to speed up the lookup operations: */
typedef struct CvStringHashNode
{
    unsigned hashval;
    CvString str;
    struct CvStringHashNode* next;
}
CvStringHashNode;

typedef struct CvGenericHash CvFileNodeHash;

typedef struct CvXMLStackRecord
{
    CvMemStoragePos pos;
    CvString struct_tag;
    int struct_indent;
    int struct_flags;
}
CvXMLStackRecord;


/** Basic element of the file storage - scalar or collection: */
typedef struct CvFileNode
{
    int tag;
    struct CvTypeInfo* info; /**< type information
            (only for user-defined object, for others it is 0) */
    union
    {
        double f; /**< scalar floating-point number */
        int i;    /**< scalar integer number */
        CvString str; /**< text string */
        CvSeq* seq; /**< sequence (ordered collection of file nodes) */
        CvFileNodeHash* map; /**< map (collection of named file nodes) */
    } data;
}
CvFileNode;

#ifdef __cplusplus
extern "C" {
#endif
typedef int (CV_CDECL *CvIsInstanceFunc)( const void* struct_ptr );
typedef void (CV_CDECL *CvReleaseFunc)( void** struct_dblptr );
typedef void* (CV_CDECL *CvReadFunc)( CvFileStorage* storage, CvFileNode* node );
typedef void (CV_CDECL *CvWriteFunc)( CvFileStorage* storage, const char* name,
                                      const void* struct_ptr, CvAttrList attributes );
typedef void* (CV_CDECL *CvCloneFunc)( const void* struct_ptr );
#ifdef __cplusplus
}
#endif

/** @brief Type information

The structure contains information about one of the standard or user-defined types. Instances of the
type may or may not contain a pointer to the corresponding CvTypeInfo structure. In any case, there
is a way to find the type info structure for a given object using the cvTypeOf function.
Alternatively, type info can be found by type name using cvFindType, which is used when an object
is read from file storage. The user can register a new type with cvRegisterType that adds the type
information structure into the beginning of the type list. Thus, it is possible to create
specialized types from generic standard types and override the basic methods.
 */
typedef struct CvTypeInfo
{
    int flags; /**< not used */
    int header_size; /**< sizeof(CvTypeInfo) */
    struct CvTypeInfo* prev; /**< previous registered type in the list */
    struct CvTypeInfo* next; /**< next registered type in the list */
    const char* type_name; /**< type name, written to file storage */
    CvIsInstanceFunc is_instance; /**< checks if the passed object belongs to the type */
    CvReleaseFunc release; /**< releases object (memory etc.) */
    CvReadFunc read; /**< reads object from file storage */
    CvWriteFunc write; /**< writes object to file storage */
    CvCloneFunc clone; /**< creates a copy of the object */
}
CvTypeInfo;


CVAPI(void*) cvLoad( const char* filename,
                     CvMemStorage* memstorage CV_DEFAULT(NULL),
                     const char* name CV_DEFAULT(NULL),
                     const char** real_name CV_DEFAULT(NULL) );




///// Additions from xmlreaderWriter.h


#define CV_FILE_STORAGE ('Y' + ('A' << 8) + ('M' << 16) + ('L' << 24))
#define CV_IS_FILE_STORAGE(fs) ((fs) != 0 && (fs)->flags == CV_FILE_STORAGE)

#define CV_CHECK_FILE_STORAGE(fs)                       \
{                                                       \
    if( !CV_IS_FILE_STORAGE(fs) )                       \
        CV_Error( (fs) ? CV_StsBadArg : CV_StsNullPtr,  \
                  "Invalid pointer to file storage" );  \
}

#define CV_CHECK_OUTPUT_FILE_STORAGE(fs)                \
{                                                       \
    CV_CHECK_FILE_STORAGE(fs);                          \
    if( !fs->write_mode )                               \
        CV_Error( CV_StsError, "The file storage is opened for reading" ); \
}


#define CV_YML_INDENT  3
#define CV_XML_INDENT  2
#define CV_YML_INDENT_FLOW  1
#define CV_FS_MAX_LEN 4096


#define CV_XML_OPENING_TAG 1
#define CV_XML_CLOSING_TAG 2
#define CV_XML_EMPTY_TAG 3
#define CV_XML_HEADER_TAG 4
#define CV_XML_DIRECTIVE_TAG 5

typedef void * gzFile;
typedef void (*CvStartWriteStruct)( struct CvFileStorage* fs, const char* key,
                                    int struct_flags, const char* type_name );
typedef void (*CvEndWriteStruct)( struct CvFileStorage* fs );
typedef void (*CvWriteInt)( struct CvFileStorage* fs, const char* key, int value );
typedef void (*CvWriteReal)( struct CvFileStorage* fs, const char* key, double value );
typedef void (*CvWriteString)( struct CvFileStorage* fs, const char* key,
                               const char* value, int quote );
typedef void (*CvWriteComment)( struct CvFileStorage* fs, const char* comment, int eol_comment );
typedef void (*CvStartNextStream)( struct CvFileStorage* fs );


typedef struct CvFileMapNode
{
    CvFileNode value;
    const CvStringHashNode* key;
    struct CvFileMapNode* next;
}
CvFileMapNode;


typedef struct CvGenericHash
{
    CV_SET_FIELDS()
    int tab_size;
    void** table;
}
CvGenericHash;

typedef CvGenericHash CvStringHash;

namespace base64
{
    class Base64Writer;

    namespace fs
    {
        enum State
        {
            Uncertain,
            NotUse,
            InUse,
        };
    }
}

typedef struct CvFileStorage
{
    int flags;
    int fmt;
    int write_mode;
    int is_first;
    CvMemStorage* memstorage;
    CvMemStorage* dststorage;
    CvMemStorage* strstorage;
    CvStringHash* str_hash;
    CvSeq* roots;
    CvSeq* write_stack;
    int struct_indent;
    int struct_flags;
    CvString struct_tag;
    int space;
    char* filename;
    FILE* file;
    gzFile gzfile;
    char* buffer;
    char* buffer_start;
    char* buffer_end;
    int wrap_margin;
    int lineno;
    int dummy_eof;
    const char* errmsg;
    char errmsgbuf[128];

    CvStartWriteStruct start_write_struct;
    CvEndWriteStruct end_write_struct;
    CvWriteInt write_int;
    CvWriteReal write_real;
    CvWriteString write_string;
    CvWriteComment write_comment;
    CvStartNextStream start_next_stream;

    const char* strbuf;
    size_t strbufsize, strbufpos;
    std::deque<char>* outbuf;
    
    base64::Base64Writer * base64_writer;
    bool is_default_using_base64;
    base64::fs::State state_of_writing_base64;  /**< used in WriteRawData only */

    bool is_write_struct_delayed;
    char* delayed_struct_key;
    int   delayed_struct_flags;
    char* delayed_type_name;

    bool is_opened;
} CvFileStorage;


namespace base64
{
    static const size_t HEADER_SIZE         = 24U;
    static const size_t ENCODED_HEADER_SIZE = 32U;

    typedef uchar uint8_t;

    extern uint8_t const base64_padding;
    extern uint8_t const base64_mapping[65];
    extern uint8_t const base64_demapping[127];

    size_t base64_encode(uint8_t const * src, uint8_t * dst, size_t off,      size_t cnt);
    size_t base64_encode(   char const * src,    char * dst, size_t off = 0U, size_t cnt = 0U);

    size_t base64_decode(uint8_t const * src, uint8_t * dst, size_t off,      size_t cnt);
    size_t base64_decode(   char const * src,    char * dst, size_t off = 0U, size_t cnt = 0U);

    bool   base64_valid (uint8_t const * src, size_t off,      size_t cnt);
    bool   base64_valid (   char const * src, size_t off = 0U, size_t cnt = 0U);

    size_t base64_encode_buffer_size(size_t cnt, bool is_end_with_zero = true);

    size_t base64_decode_buffer_size(size_t cnt, bool is_end_with_zero = true);
    size_t base64_decode_buffer_size(size_t cnt, char  const * src, bool is_end_with_zero = true);
    size_t base64_decode_buffer_size(size_t cnt, uchar const * src, bool is_end_with_zero = true);



    template<typename _uint_t>      inline size_t to_binary(_uint_t       val, uchar * cur);
    template<>                      inline size_t to_binary(double        val, uchar * cur);
    template<>                      inline size_t to_binary(float         val, uchar * cur);
    template<typename _primitive_t> inline size_t to_binary(uchar const * val, uchar * cur);

    template<typename _uint_t>      inline size_t binary_to(uchar const * cur, _uint_t & val);
    template<>                      inline size_t binary_to(uchar const * cur, double  & val);
    template<>                      inline size_t binary_to(uchar const * cur, float   & val);
    template<typename _primitive_t> inline size_t binary_to(uchar const * cur, uchar   * val);

    class RawDataToBinaryConvertor;

    class BinaryToCvSeqConvertor;



    class Base64ContextParser
    {
    public:
        explicit Base64ContextParser(uchar * buffer, size_t size);
        ~Base64ContextParser();
        Base64ContextParser & read(const uchar * beg, const uchar * end);
        bool flush();
    private:
        static const size_t BUFFER_LEN = 120U;
        uchar * dst_cur;
        uchar * dst_end;
        std::vector<uchar> base64_buffer;
        uchar * src_beg;
        uchar * src_cur;
        uchar * src_end;
        std::vector<uchar> binary_buffer;
    };

    class Base64ContextEmitter;

    class Base64Writer
    {
    public:
        Base64Writer(::CvFileStorage * fs);
        ~Base64Writer();
        void write(const void* _data, size_t len, const char* dt);
        template<typename _to_binary_convertor_t> void write(_to_binary_convertor_t & convertor, const char* dt);

    private:
        void check_dt(const char* dt);

    private:
        // disable copy and assignment
        Base64Writer(const Base64Writer &);
        Base64Writer & operator=(const Base64Writer &);

    private:

        Base64ContextEmitter * emitter;
        std::string data_type_string;
    };
	

    std::string make_base64_header(const char * dt);

    bool read_base64_header(std::vector<char> const & header, std::string & dt);

    void make_seq(void * binary_data, int elem_cnt, const char * dt, CvSeq & seq);

    void cvWriteRawDataBase64(::CvFileStorage* fs, const void* _data, int len, const char* dt);
}

void icvPuts( CvFileStorage* fs, const char* str );
char* icvGets( CvFileStorage* fs, char* str, int maxCount);
void icvCloseFile( CvFileStorage* fs );
void icvRewind( CvFileStorage* fs );

CvGenericHash* cvCreateMap( int flags, int header_size, int elem_size,
             CvMemStorage* storage, int start_tab_size );

