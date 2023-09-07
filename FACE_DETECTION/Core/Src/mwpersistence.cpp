/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#include <ctype.h>
#include <deque>
#include <sstream>
#include <string>
#include <iterator>

#include "mwcore.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/core/core_c.h"

typedef void* gzFile;


/****************************************************************************************\
*                            Common macros and type definitions                          *
\****************************************************************************************/

#if defined __GNUC__ && !defined __EXCEPTIONS
#define CV_TRY
#define CV_CATCH(A, B) for (A B; false; )
#define CV_CATCH_ALL if (false)
#define CV_THROW(A) abort()
#define CV_RETHROW() abort()
#else
#define CV_TRY try
#define CV_CATCH(A, B) catch(const A & B)
#define CV_CATCH_ALL catch(...)
#define CV_THROW(A) throw A
#define CV_RETHROW() throw
#endif



#define cv_isprint(c)     ((uchar)(c) >= (uchar)' ')
#define cv_isprint_or_tab(c)  ((uchar)(c) >= (uchar)' ' || (c) == '\t')
#define CV_MAX_DIM_HEAP       1024
#define CV_BADARG_ERR -49

static inline bool cv_isalnum(char c)
{
    return ('0' <= c && c <= '9') || ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

static inline bool cv_isalpha(char c)
{
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

static inline bool cv_isdigit(char c)
{
    return '0' <= c && c <= '9';
}

static inline bool cv_isspace(char c)
{
    return (9 <= c && c <= 13) || c == ' ';
}

static char* icv_itoa( int _val, char* buffer, int /*radix*/ )
{
    const int radix = 10;
    char* ptr=buffer + 23 /* enough even for 64-bit integers */;
    unsigned val = abs(_val);

    *ptr = '\0';
    do
    {
        unsigned r = val / radix;
        *--ptr = (char)(val - (r*radix) + '0');
        val = r;
    }
    while( val != 0 );

    if( _val < 0 )
        *--ptr = '-';

    return ptr;
}

static inline char* cv_skip_BOM(char* ptr)
{
    if((uchar)ptr[0] == 0xef && (uchar)ptr[1] == 0xbb && (uchar)ptr[2] == 0xbf) //UTF-8 BOM
    {
      return ptr + 3;
    }
    return ptr;
}

static inline bool cv_strcasecmp(const char * s1, const char * s2)
{
    if ( s1 == 0 && s2 == 0 )
        return true;
    else if ( s1 == 0 || s2 == 0 )
        return false;

    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);
    if ( len1 != len2 )
        return false;

    for ( size_t i = 0U; i < len1; i++ )
        if ( tolower( static_cast<int>(s1[i]) ) != tolower( static_cast<int>(s2[i]) ) )
            return false;

    return true;
}

cv::String cv::FileStorage::getDefaultObjectName(const cv::String& _filename)
{
    static const char* stubname = "unnamed";
    const char* filename = _filename.c_str();
    const char* ptr2 = filename + _filename.size();
    const char* ptr = ptr2 - 1;
    cv::AutoBuffer<char> name_buf(_filename.size()+1);

    while( ptr >= filename && *ptr != '\\' && *ptr != '/' && *ptr != ':' )
    {
        if( *ptr == '.' && (!*ptr2 || strncmp(ptr2, ".gz", 3) == 0) )
            ptr2 = ptr;
        ptr--;
    }
    ptr++;
    if( ptr == ptr2 )
        CV_Error( CV_StsBadArg, "Invalid filename" );

    char* name = name_buf;

    // name must start with letter or '_'
    if( !cv_isalpha(*ptr) && *ptr!= '_' ){
        *name++ = '_';
    }

    while( ptr < ptr2 )
    {
        char c = *ptr++;
        if( !cv_isalnum(c) && c != '-' && c != '_' )
            c = '_';
        *name++ = c;
    }
    *name = '\0';
    name = name_buf;
    if( strcmp( name, "_" ) == 0 )
        strcpy( name, stubname );
    return String(name);
}


void icvPuts( CvFileStorage* fs, const char* str )
{
    if( fs->outbuf )
        std::copy(str, str + strlen(str), std::back_inserter(*fs->outbuf));
    else if( fs->file )
        fputs( str, fs->file );
#if USE_ZLIB
    else if( fs->gzfile )
        gzputs( fs->gzfile, str );
#endif
    else
        CV_Error( CV_StsError, "The storage is not opened" );
}

char* icvGets( CvFileStorage* fs, char* str, int maxCount )
{
    if( fs->strbuf )
    {
        size_t i = fs->strbufpos, len = fs->strbufsize;
        int j = 0;
        const char* instr = fs->strbuf;
        while( i < len && j < maxCount-1 )
        {
            char c = instr[i++];
            if( c == '\0' )
                break;
            str[j++] = c;
            if( c == '\n' )
                break;
        }
        str[j++] = '\0';
        fs->strbufpos = i;
        return j > 1 ? str : 0;
    }
    if( fs->file )
	{
        return fgets( str, maxCount, fs->file );
	}
	else
	{
        CV_Error(CV_StsError,"The storage is not opened");
//         return 0;
	}
}

int icvEof( CvFileStorage* fs )
{
    if( fs->strbuf )
        return fs->strbufpos >= fs->strbufsize;
    if( fs->file )
        return feof(fs->file);
#if USE_ZLIB
    if( fs->gzfile )
        return gzeof(fs->gzfile);
#endif
    return false;
}

void icvCloseFile( CvFileStorage* fs )
{
    if( fs->file )
        fclose( fs->file );
#if USE_ZLIB
    else if( fs->gzfile )
        gzclose( fs->gzfile );
#endif
    fs->file = 0;
    fs->gzfile = 0;
    fs->strbuf = 0;
    fs->strbufpos = 0;
    fs->is_opened = false;
}

void icvRewind( CvFileStorage* fs )
{
    if( fs->file )
        rewind(fs->file);
#if USE_ZLIB
    else if( fs->gzfile )
        gzrewind(fs->gzfile);
#endif
    fs->strbufpos = 0;
}




CV_IMPL const char*
cvAttrValue( const CvAttrList* attr, const char* attr_name )
{
    while( attr && attr->attr )
    {
        int i;
        for( i = 0; attr->attr[i*2] != 0; i++ )
        {
            if( strcmp( attr_name, attr->attr[i*2] ) == 0 )
                return attr->attr[i*2+1];
        }
        attr = attr->next;
    }

    return 0;
}


CvGenericHash*
cvCreateMap( int flags, int header_size, int elem_size,
             CvMemStorage* storage, int start_tab_size )
{
    if( header_size < (int)sizeof(CvGenericHash) )
        CV_Error( CV_StsBadSize, "Too small map header_size" );

    if( start_tab_size <= 0 )
        start_tab_size = 16;

    CvGenericHash* map = (CvGenericHash*)cvCreateSet( flags, header_size, elem_size, storage );

    map->tab_size = start_tab_size;
    start_tab_size *= sizeof(map->table[0]);
    map->table = (void**)cvMemStorageAlloc( storage, start_tab_size );
    memset( map->table, 0, start_tab_size );

    return map;
}

#define CV_PARSE_ERROR( errmsg )                                    \
    icvParseError( fs, CV_Func, (errmsg), __FILE__, __LINE__ )

static void
icvParseError( CvFileStorage* fs, const char* func_name,
               const char* err_msg, const char* source_file, int source_line )
{
    char buf[1<<10];
    sprintf( buf, "%s(%d): %s", fs->filename, fs->lineno, err_msg );
    cvError( CV_StsParseError, func_name, buf, source_file, source_line );
}


static void
icvFSCreateCollection( CvFileStorage* fs, int tag, CvFileNode* collection )
{
    if( CV_NODE_IS_MAP(tag) )
    {
        if( collection->tag != CV_NODE_NONE )
        {
            assert( fs->fmt == CV_STORAGE_FORMAT_XML );
            CV_PARSE_ERROR( "Sequence element should not have name (use <_></_>)" );
        }

        collection->data.map = cvCreateMap( 0, sizeof(CvFileNodeHash),
                            sizeof(CvFileMapNode), fs->memstorage, 16 );
    }
    else
    {
        CvSeq* seq;
        seq = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvFileNode), fs->memstorage );

        // if <collection> contains some scalar element, add it to the newly created collection
        if( CV_NODE_TYPE(collection->tag) != CV_NODE_NONE )
            cvSeqPush( seq, collection );

        collection->data.seq = seq;
    }

    collection->tag = tag;
    cvSetSeqBlockSize( collection->data.seq, 8 );
}


static char*
icvFSDoResize( CvFileStorage* fs, char* ptr, int len )
{
    char* new_ptr = 0;
    int written_len = (int)(ptr - fs->buffer_start);
    int new_size = (int)((fs->buffer_end - fs->buffer_start)*3/2);
    new_size = MAX( written_len + len, new_size );
    new_ptr = (char*)cvAlloc( new_size + 256 );
    fs->buffer = new_ptr + (fs->buffer - fs->buffer_start);
    if( written_len > 0 )
        memcpy( new_ptr, fs->buffer_start, written_len );
    fs->buffer_start = new_ptr;
    fs->buffer_end = fs->buffer_start + new_size;
    new_ptr += written_len;
    return new_ptr;
}


inline char* icvFSResizeWriteBuffer( CvFileStorage* fs, char* ptr, int len )
{
    return ptr + len < fs->buffer_end ? ptr : icvFSDoResize( fs, ptr, len );
}


static char*
icvFSFlush( CvFileStorage* fs )
{
    char* ptr = fs->buffer;
    int indent;

    if( ptr > fs->buffer_start + fs->space )
    {
        ptr[0] = '\n';
        ptr[1] = '\0';
        icvPuts( fs, fs->buffer_start );
        fs->buffer = fs->buffer_start;
    }

    indent = fs->struct_indent;

    if( fs->space != indent )
    {
        memset( fs->buffer_start, ' ', indent );
        fs->space = indent;
    }

    ptr = fs->buffer = fs->buffer_start + fs->space;

    return ptr;
}


static void
icvClose( CvFileStorage* fs, cv::String* out )
{
    if( out )
        out->clear();

    if( !fs )
        CV_Error( CV_StsNullPtr, "NULL double pointer to file storage" );

    if( fs->is_opened )
    {
        if( fs->write_mode && (fs->file || fs->gzfile || fs->outbuf) )
        {
            if( fs->write_stack )
            {
                while( fs->write_stack->total > 0 )
                    cvEndWriteStruct(fs);
            }
            icvFSFlush(fs);
            if( fs->fmt == CV_STORAGE_FORMAT_XML )
                icvPuts( fs, "</opencv_storage>\n" );
            else if ( fs->fmt == CV_STORAGE_FORMAT_JSON )
                icvPuts( fs, "}\n" );
        }

        icvCloseFile(fs);
    }

    if( fs->outbuf && out )
    {
        *out = cv::String(fs->outbuf->begin(), fs->outbuf->end());
    }
}


/* closes file storage and deallocates buffers */
CV_IMPL  void
cvReleaseFileStorage( CvFileStorage** p_fs )
{
    if( !p_fs )
        CV_Error( CV_StsNullPtr, "NULL double pointer to file storage" );

    if( *p_fs )
    {
        CvFileStorage* fs = *p_fs;
        *p_fs = 0;

        icvClose(fs, 0);

        cvReleaseMemStorage( &fs->strstorage );
        cvFree( &fs->buffer_start );
        cvReleaseMemStorage( &fs->memstorage );

        delete fs->outbuf;
        delete fs->base64_writer;
        delete[] fs->delayed_struct_key;
        delete[] fs->delayed_type_name;

        memset( fs, 0, sizeof(*fs) );
        cvFree( &fs );
    }
}


#define CV_HASHVAL_SCALE 33

CV_IMPL CvStringHashNode*
cvGetHashedKey( CvFileStorage* fs, const char* str, int len, int create_missing )
{
    CvStringHashNode* node = 0;
    unsigned hashval = 0;
    int i, tab_size;

    if( !fs )
        return 0;

    CvStringHash* map = fs->str_hash;

    if( len < 0 )
    {
        for( i = 0; str[i] != '\0'; i++ )
            hashval = hashval*CV_HASHVAL_SCALE + (unsigned char)str[i];
        len = i;
    }
    else for( i = 0; i < len; i++ )
        hashval = hashval*CV_HASHVAL_SCALE + (unsigned char)str[i];

    hashval &= INT_MAX;
    tab_size = map->tab_size;
    if( (tab_size & (tab_size - 1)) == 0 )
        i = (int)(hashval & (tab_size - 1));
    else
        i = (int)(hashval % tab_size);

    for( node = (CvStringHashNode*)(map->table[i]); node != 0; node = node->next )
    {
        if( node->hashval == hashval &&
            node->str.len == len &&
            memcmp( node->str.ptr, str, len ) == 0 )
            break;
    }

    if( !node && create_missing )
    {
        node = (CvStringHashNode*)cvSetNew( (CvSet*)map );
        node->hashval = hashval;
        node->str = cvMemStorageAllocString( map->storage, str, len );
        node->next = (CvStringHashNode*)(map->table[i]);
        map->table[i] = node;
    }

    return node;
}


CV_IMPL CvFileNode*
cvGetFileNode( CvFileStorage* fs, CvFileNode* _map_node,
               const CvStringHashNode* key,
               int create_missing )
{
    CvFileNode* value = 0;
    int k = 0, attempts = 1;

    if( !fs )
        return 0;

    CV_CHECK_FILE_STORAGE(fs);

    if( !key )
        CV_Error( CV_StsNullPtr, "Null key element" );

    if( _map_node )
    {
        if( !fs->roots )
            return 0;
        attempts = fs->roots->total;
    }

    for( k = 0; k < attempts; k++ )
    {
        int i, tab_size;
        CvFileNode* map_node = _map_node;
        CvFileMapNode* another;
        CvFileNodeHash* map;

        if( !map_node )
            map_node = (CvFileNode*)cvGetSeqElem( fs->roots, k );
        CV_Assert(map_node != NULL);
        if( !CV_NODE_IS_MAP(map_node->tag) )
        {
            if( (!CV_NODE_IS_SEQ(map_node->tag) || map_node->data.seq->total != 0) &&
                CV_NODE_TYPE(map_node->tag) != CV_NODE_NONE )
                CV_Error( CV_StsError, "The node is neither a map nor an empty collection" );
            return 0;
        }

        map = map_node->data.map;
        tab_size = map->tab_size;

        if( (tab_size & (tab_size - 1)) == 0 )
            i = (int)(key->hashval & (tab_size - 1));
        else
            i = (int)(key->hashval % tab_size);

        for( another = (CvFileMapNode*)(map->table[i]); another != 0; another = another->next )
            if( another->key == key )
            {
                if( !create_missing )
                {
                    value = &another->value;
                    return value;
                }
                CV_PARSE_ERROR( "Duplicated key" );
            }

        if( k == attempts - 1 && create_missing )
        {
            CvFileMapNode* node = (CvFileMapNode*)cvSetNew( (CvSet*)map );
            node->key = key;

            node->next = (CvFileMapNode*)(map->table[i]);
            map->table[i] = node;
            value = (CvFileNode*)node;
        }
    }

    return value;
}


CV_IMPL CvFileNode*
cvGetFileNodeByName( const CvFileStorage* fs, const CvFileNode* _map_node, const char* str )
{
    CvFileNode* value = 0;
    int i, len, tab_size;
    unsigned hashval = 0;
    int k = 0, attempts = 1;

    if( !fs )
        return 0;

    CV_CHECK_FILE_STORAGE(fs);

    if( !str )
        CV_Error( CV_StsNullPtr, "Null element name" );

    for( i = 0; str[i] != '\0'; i++ )
        hashval = hashval*CV_HASHVAL_SCALE + (unsigned char)str[i];
    hashval &= INT_MAX;
    len = i;

    if( !_map_node )
    {
        if( !fs->roots )
            return 0;
        attempts = fs->roots->total;
    }

    for( k = 0; k < attempts; k++ )
    {
        CvFileNodeHash* map;
        const CvFileNode* map_node = _map_node;
        CvFileMapNode* another;

        if( !map_node )
            map_node = (CvFileNode*)cvGetSeqElem( fs->roots, k );

        if( !CV_NODE_IS_MAP(map_node->tag) )
        {
            if( (!CV_NODE_IS_SEQ(map_node->tag) || map_node->data.seq->total != 0) &&
                CV_NODE_TYPE(map_node->tag) != CV_NODE_NONE )
                CV_Error( CV_StsError, "The node is neither a map nor an empty collection" );
            return 0;
        }

        map = map_node->data.map;
        tab_size = map->tab_size;

        if( (tab_size & (tab_size - 1)) == 0 )
            i = (int)(hashval & (tab_size - 1));
        else
            i = (int)(hashval % tab_size);

        for( another = (CvFileMapNode*)(map->table[i]); another != 0; another = another->next )
        {
            const CvStringHashNode* key = another->key;

            if( key->hashval == hashval &&
                key->str.len == len &&
                memcmp( key->str.ptr, str, len ) == 0 )
            {
                value = &another->value;
                return value;
            }
        }
    }

    return value;
}


CV_IMPL CvFileNode*
cvGetRootFileNode( const CvFileStorage* fs, int stream_index )
{
    CV_CHECK_FILE_STORAGE(fs);

    if( !fs->roots || (unsigned)stream_index >= (unsigned)fs->roots->total )
        return 0;

    return (CvFileNode*)cvGetSeqElem( fs->roots, stream_index );
}


static char*
icvDoubleToString( char* buf, double value )
{
    Cv64suf val;
    unsigned ieee754_hi;

    val.f = value;
    ieee754_hi = (unsigned)(val.u >> 32);

    if( (ieee754_hi & 0x7ff00000) != 0x7ff00000 )
    {
        int ivalue = cvRound(value);
        if( ivalue == value )
            sprintf( buf, "%d.", ivalue );
        else
        {
            static const char* fmt = "%.16e";
            char* ptr = buf;
            sprintf( buf, fmt, value );
            if( *ptr == '+' || *ptr == '-' )
                ptr++;
            for( ; cv_isdigit(*ptr); ptr++ )
                ;
            if( *ptr == ',' )
                *ptr = '.';
        }
    }
    else
    {
        unsigned ieee754_lo = (unsigned)val.u;
        if( (ieee754_hi & 0x7fffffff) + (ieee754_lo != 0) > 0x7ff00000 )
            strcpy( buf, ".Nan" );
        else
            strcpy( buf, (int)ieee754_hi < 0 ? "-.Inf" : ".Inf" );
    }

    return buf;
}


static void
icvProcessSpecialDouble( CvFileStorage* fs, char* buf, double* value, char** endptr )
{
    char c = buf[0];
    int inf_hi = 0x7ff00000;

    if( c == '-' || c == '+' )
    {
        inf_hi = c == '-' ? 0xfff00000 : 0x7ff00000;
        c = *++buf;
    }

    if( c != '.' )
        CV_PARSE_ERROR( "Bad format of floating-point constant" );

    union{double d; uint64 i;} v;
    v.d = 0.;
    if( toupper(buf[1]) == 'I' && toupper(buf[2]) == 'N' && toupper(buf[3]) == 'F' )
        v.i = (uint64)inf_hi << 32;
    else if( toupper(buf[1]) == 'N' && toupper(buf[2]) == 'A' && toupper(buf[3]) == 'N' )
        v.i = (uint64)-1;
    else
        CV_PARSE_ERROR( "Bad format of floating-point constant" );
    *value = v.d;

    *endptr = buf + 4;
}


static double icv_strtod( CvFileStorage* fs, char* ptr, char** endptr )
{
    double fval = strtod( ptr, endptr );
    if( **endptr == '.' )
    {
        char* dot_pos = *endptr;
        *dot_pos = ',';
        double fval2 = strtod( ptr, endptr );
        *dot_pos = '.';
        if( *endptr > dot_pos )
            fval = fval2;
        else
            *endptr = dot_pos;
    }

    if( *endptr == ptr || cv_isalpha(**endptr) )
        icvProcessSpecialDouble( fs, ptr, &fval, endptr );

    return fval;
}

// this function will convert "aa?bb&cc&dd" to {"aa", "bb", "cc", "dd"}
static std::vector<std::string> analyze_file_name( std::string const & file_name )
{
    static const char not_file_name       = '\n';
    static const char parameter_begin     = '?';
    static const char parameter_separator = '&';
    std::vector<std::string> result;

    if ( file_name.find(not_file_name, 0U) != std::string::npos )
        return result;

    size_t beg = file_name.find_last_of(parameter_begin);
    size_t end = file_name.size();
    result.push_back(file_name.substr(0U, beg));

    if ( beg != std::string::npos )
    {
        beg ++;
        for ( size_t param_beg = beg, param_end = beg;
              param_end < end;
              param_beg = param_end + 1U )
        {
            param_end = file_name.find_first_of( parameter_separator, param_beg );
            if ( (param_end == std::string::npos || param_end != param_beg) && param_beg + 1U < end )
            {
                result.push_back( file_name.substr( param_beg, param_end - param_beg ) );
            }
        }
    }

    return result;
}

static bool is_param_exist( const std::vector<std::string> & params, const std::string & param )
{
    if ( params.size() < 2U )
        return false;

    return std::find(params.begin(), params.end(), param) != params.end();
}

static void switch_to_Base64_state( CvFileStorage* fs, base64::fs::State state )
{
    const char * err_unkonwn_state = "Unexpected error, unable to determine the Base64 state.";
    const char * err_unable_to_switch = "Unexpected error, unable to switch to this state.";

    /* like a finite state machine */
    switch (fs->state_of_writing_base64)
    {
    case base64::fs::Uncertain:
        switch (state)
        {
        case base64::fs::InUse:
            CV_DbgAssert( fs->base64_writer == 0 );
            fs->base64_writer = new base64::Base64Writer( fs );
            break;
        case base64::fs::Uncertain:
            break;
        case base64::fs::NotUse:
            break;
        default:
            CV_Error( CV_StsError, err_unkonwn_state );
            break;
        }
        break;
    case base64::fs::InUse:
        switch (state)
        {
        case base64::fs::InUse:
        case base64::fs::NotUse:
            CV_Error( CV_StsError, err_unable_to_switch );
            break;
        case base64::fs::Uncertain:
            delete fs->base64_writer;
            fs->base64_writer = 0;
            break;
        default:
            CV_Error( CV_StsError, err_unkonwn_state );
            break;
        }
        break;
    case base64::fs::NotUse:
        switch (state)
        {
        case base64::fs::InUse:
        case base64::fs::NotUse:
            CV_Error( CV_StsError, err_unable_to_switch );
            break;
        case base64::fs::Uncertain:
            break;
        default:
            CV_Error( CV_StsError, err_unkonwn_state );
            break;
        }
        break;
    default:
        CV_Error( CV_StsError, err_unkonwn_state );
        break;
    }

    fs->state_of_writing_base64 = state;
}


static void check_if_write_struct_is_delayed( CvFileStorage* fs, bool change_type_to_base64 = false )
{
    if ( fs->is_write_struct_delayed )
    {
        /* save data to prevent recursive call errors */
        std::string struct_key;
        std::string type_name;
        int struct_flags = fs->delayed_struct_flags;

        if ( fs->delayed_struct_key != 0 && *fs->delayed_struct_key != '\0' )
        {
            struct_key.assign(fs->delayed_struct_key);
        }
        if ( fs->delayed_type_name != 0 && *fs->delayed_type_name != '\0' )
        {
            type_name.assign(fs->delayed_type_name);
        }

        /* reset */
        delete[] fs->delayed_struct_key;
        delete[] fs->delayed_type_name;
        fs->delayed_struct_key   = 0;
        fs->delayed_struct_flags = 0;
        fs->delayed_type_name    = 0;

        fs->is_write_struct_delayed = false;

        /* call */
        if ( change_type_to_base64 )
        {
            fs->start_write_struct( fs, struct_key.c_str(), struct_flags, "binary");
            if ( fs->state_of_writing_base64 != base64::fs::Uncertain )
                switch_to_Base64_state( fs, base64::fs::Uncertain );
            switch_to_Base64_state( fs, base64::fs::InUse );
        }
        else
        {
            fs->start_write_struct( fs, struct_key.c_str(), struct_flags, type_name.c_str());
            if ( fs->state_of_writing_base64 != base64::fs::Uncertain )
                switch_to_Base64_state( fs, base64::fs::Uncertain );
            switch_to_Base64_state( fs, base64::fs::NotUse );
        }
    }
}

static const size_t PARSER_BASE64_BUFFER_SIZE = 1024U * 1024U / 8U;

static int icvCalcStructSize( const char* dt, int initial_size );


/****************************************************************************************\
*                                       XML Parser                                       *
\****************************************************************************************/

#define CV_XML_INSIDE_COMMENT 1
#define CV_XML_INSIDE_TAG 2
#define CV_XML_INSIDE_DIRECTIVE 3

static char*
icvXMLSkipSpaces( CvFileStorage* fs, char* ptr, int mode )
{
    int level = 0;

    for(;;)
    {
        char c;
        ptr--;

        if( mode == CV_XML_INSIDE_COMMENT )
        {
            do c = *++ptr;
            while( cv_isprint_or_tab(c) && (c != '-' || ptr[1] != '-' || ptr[2] != '>') );

            if( c == '-' )
            {
                assert( ptr[1] == '-' && ptr[2] == '>' );
                mode = 0;
                ptr += 3;
            }
        }
        else if( mode == CV_XML_INSIDE_DIRECTIVE )
        {
            // !!!NOTE!!! This is not quite correct, but should work in most cases
            do
            {
                c = *++ptr;
                level += c == '<';
                level -= c == '>';
                if( level < 0 )
                    return ptr;
            } while( cv_isprint_or_tab(c) );
        }
        else
        {
            do c = *++ptr;
            while( c == ' ' || c == '\t' );

            if( c == '<' && ptr[1] == '!' && ptr[2] == '-' && ptr[3] == '-' )
            {
                if( mode != 0 )
                    CV_PARSE_ERROR( "Comments are not allowed here" );
                mode = CV_XML_INSIDE_COMMENT;
                ptr += 4;
            }
            else if( cv_isprint(c) )
                break;
        }

        if( !cv_isprint(*ptr) )
        {
            int max_size = (int)(fs->buffer_end - fs->buffer_start);
            if( *ptr != '\0' && *ptr != '\n' && *ptr != '\r' )
                CV_PARSE_ERROR( "Invalid character in the stream" );
            ptr = icvGets( fs, fs->buffer_start, max_size );
            if( !ptr )
            {
                ptr = fs->buffer_start;
                *ptr = '\0';
                fs->dummy_eof = 1;
                break;
            }
            else
            {
                int l = (int)strlen(ptr);
                if( ptr[l-1] != '\n' && ptr[l-1] != '\r' && !icvEof(fs) )
                    CV_PARSE_ERROR( "Too long string or a last string w/o newline" );
            }
            fs->lineno++;
        }
    }
    return ptr;
}


static void icvXMLGetMultilineStringContent(CvFileStorage* fs,
    char* ptr, char* &beg, char* &end)
{
    ptr = icvXMLSkipSpaces(fs, ptr, CV_XML_INSIDE_TAG);
    beg = ptr;
    end = ptr;
    if ( fs->dummy_eof )
        return ; /* end of file */

    if ( *beg == '<' )
        return; /* end of string */

    /* find end */
    while( cv_isprint(*ptr) ) /* no check for base64 string */
        ++ ptr;
    if ( *ptr == '\0' )
        CV_PARSE_ERROR( "Unexpected end of line" );

    end = ptr;
}


static char* icvXMLParseBase64(CvFileStorage* fs, char* ptr, CvFileNode * node)
{
    char * beg = 0;
    char * end = 0;

    icvXMLGetMultilineStringContent(fs, ptr, beg, end);
    if (beg >= end)
        return end; // CV_PARSE_ERROR("Empty Binary Data");

    /* calc (decoded) total_byte_size from header */
    std::string dt;
    {
        if (end - beg < static_cast<int>(base64::ENCODED_HEADER_SIZE))
            CV_PARSE_ERROR("Unrecognized Base64 header");

        std::vector<char> header(base64::HEADER_SIZE + 1, ' ');
        base64::base64_decode(beg, header.data(), 0U, base64::ENCODED_HEADER_SIZE);
        if ( !base64::read_base64_header(header, dt) || dt.empty() )
            CV_PARSE_ERROR("Invalid `dt` in Base64 header");

        beg += base64::ENCODED_HEADER_SIZE;
    }

    /* get all Base64 data */
    std::string base64_buffer; // not an efficient way.
    base64_buffer.reserve( PARSER_BASE64_BUFFER_SIZE );
    while( beg < end )
    {
        base64_buffer.append( beg, end );
        beg = end;
        icvXMLGetMultilineStringContent( fs, beg, beg, end );
    }
    if ( base64_buffer.empty() ||
         !base64::base64_valid(base64_buffer.data(), 0U, base64_buffer.size()) )
        CV_PARSE_ERROR( "Invalid Base64 data." );

    /* alloc buffer for all decoded data(include header) */
    std::vector<uchar> binary_buffer( base64::base64_decode_buffer_size(base64_buffer.size()) );
    int total_byte_size = static_cast<int>(
        base64::base64_decode_buffer_size( base64_buffer.size(), base64_buffer.data(), false )
        );
    {
        base64::Base64ContextParser parser(binary_buffer.data(), binary_buffer.size() );
        const uchar * buffer_beg = reinterpret_cast<const uchar *>( base64_buffer.data() );
        const uchar * buffer_end = buffer_beg + base64_buffer.size();
        parser.read( buffer_beg, buffer_end );
        parser.flush();
    }

    /* save as CvSeq */
    int elem_size = ::icvCalcStructSize(dt.c_str(), 0);
    if (total_byte_size % elem_size != 0)
        CV_PARSE_ERROR("data size not matches elememt size");
    int elem_cnt = total_byte_size / elem_size;

    node->tag = CV_NODE_NONE;
    int struct_flags = CV_NODE_SEQ;
    /* after icvFSCreateCollection, node->tag == struct_flags */
    icvFSCreateCollection(fs, struct_flags, node);
    base64::make_seq(binary_buffer.data(), elem_cnt, dt.c_str(), *node->data.seq);

    if (fs->dummy_eof) {
        /* end of file */
        return fs->buffer_start;
    } else {
        /* end of line */
        return end;
    }
}


static char*
icvXMLParseTag( CvFileStorage* fs, char* ptr, CvStringHashNode** _tag,
                CvAttrList** _list, int* _tag_type );

static char*
icvXMLParseValue( CvFileStorage* fs, char* ptr, CvFileNode* node,
                  int value_type CV_DEFAULT(CV_NODE_NONE))
{
    CvFileNode *elem = node;
    bool have_space = true, is_simple = true;
    int is_user_type = CV_NODE_IS_USER(value_type);
    memset( node, 0, sizeof(*node) );

    value_type = CV_NODE_TYPE(value_type);

    for(;;)
    {
        char c = *ptr, d;
        char* endptr;

        if( cv_isspace(c) || c == '\0' || (c == '<' && ptr[1] == '!' && ptr[2] == '-') )
        {
            ptr = icvXMLSkipSpaces( fs, ptr, 0 );
            have_space = true;
            c = *ptr;
        }

        d = ptr[1];

        if( c =='<' || c == '\0' )
        {
            CvStringHashNode *key = 0, *key2 = 0;
            CvAttrList* list = 0;
            CvTypeInfo* info = 0;
            int tag_type = 0;
            int is_noname = 0;
            const char* type_name = 0;
            int elem_type = CV_NODE_NONE;

            if( d == '/' || c == '\0' )
                break;

            ptr = icvXMLParseTag( fs, ptr, &key, &list, &tag_type );

            if( tag_type == CV_XML_DIRECTIVE_TAG )
                CV_PARSE_ERROR( "Directive tags are not allowed here" );
            if( tag_type == CV_XML_EMPTY_TAG )
                CV_PARSE_ERROR( "Empty tags are not supported" );

            assert( tag_type == CV_XML_OPENING_TAG );

            /* for base64 string */
            bool is_binary_string = false;

            type_name = list ? cvAttrValue( list, "type_id" ) : 0;
            if( type_name )
            {
                if( strcmp( type_name, "str" ) == 0 )
                    elem_type = CV_NODE_STRING;
                else if( strcmp( type_name, "map" ) == 0 )
                    elem_type = CV_NODE_MAP;
                else if( strcmp( type_name, "seq" ) == 0 )
                    elem_type = CV_NODE_SEQ;
                else if (strcmp(type_name, "binary") == 0)
                {
                    elem_type = CV_NODE_NONE;
                    is_binary_string = true;
                }
                else
                {
                    info = cvFindType( type_name );
                    if( info )
                        elem_type = CV_NODE_USER;
                }
            }

            is_noname = key->str.len == 1 && key->str.ptr[0] == '_';
            if( !CV_NODE_IS_COLLECTION(node->tag) )
            {
                icvFSCreateCollection( fs, is_noname ? CV_NODE_SEQ : CV_NODE_MAP, node );
            }
            else if( is_noname ^ CV_NODE_IS_SEQ(node->tag) )
                CV_PARSE_ERROR( is_noname ? "Map element should have a name" :
                              "Sequence element should not have name (use <_></_>)" );

            if( is_noname )
                elem = (CvFileNode*)cvSeqPush( node->data.seq, 0 );
            else
                elem = cvGetFileNode( fs, node, key, 1 );
            CV_Assert(elem);
            if (!is_binary_string)
                ptr = icvXMLParseValue( fs, ptr, elem, elem_type);
            else {
                /* for base64 string */
                ptr = icvXMLParseBase64( fs, ptr, elem);
                ptr = icvXMLSkipSpaces( fs, ptr, 0 );
            }

            if( !is_noname )
                elem->tag |= CV_NODE_NAMED;
            is_simple = is_simple && !CV_NODE_IS_COLLECTION(elem->tag);
            elem->info = info;
            ptr = icvXMLParseTag( fs, ptr, &key2, &list, &tag_type );
            if( tag_type != CV_XML_CLOSING_TAG || key2 != key )
                CV_PARSE_ERROR( "Mismatched closing tag" );
            have_space = true;
        }
        else
        {
            if( !have_space )
                CV_PARSE_ERROR( "There should be space between literals" );

            elem = node;
            if( node->tag != CV_NODE_NONE )
            {
                if( !CV_NODE_IS_COLLECTION(node->tag) )
                    icvFSCreateCollection( fs, CV_NODE_SEQ, node );

                elem = (CvFileNode*)cvSeqPush( node->data.seq, 0 );
                elem->info = 0;
            }

            if( value_type != CV_NODE_STRING &&
                (cv_isdigit(c) || ((c == '-' || c == '+') &&
                (cv_isdigit(d) || d == '.')) || (c == '.' && cv_isalnum(d))) ) // a number
            {
                double fval;
                int ival;
                endptr = ptr + (c == '-' || c == '+');
                while( cv_isdigit(*endptr) )
                    endptr++;
                if( *endptr == '.' || *endptr == 'e' )
                {
                    fval = icv_strtod( fs, ptr, &endptr );
                    /*if( endptr == ptr || cv_isalpha(*endptr) )
                        icvProcessSpecialDouble( fs, ptr, &fval, &endptr ));*/
                    elem->tag = CV_NODE_REAL;
                    elem->data.f = fval;
                }
                else
                {
                    ival = (int)strtol( ptr, &endptr, 0 );
                    elem->tag = CV_NODE_INT;
                    elem->data.i = ival;
                }

                if( endptr == ptr )
                    CV_PARSE_ERROR( "Invalid numeric value (inconsistent explicit type specification?)" );

                ptr = endptr;
            }
            else
            {
                // string
                char buf[CV_FS_MAX_LEN+16] = {0};
                int i = 0, len, is_quoted = 0;
                elem->tag = CV_NODE_STRING;
                if( c == '\"' )
                    is_quoted = 1;
                else
                    --ptr;

                for( ;; )
                {
                    c = *++ptr;
                    if( !cv_isalnum(c) )
                    {
                        if( c == '\"' )
                        {
                            if( !is_quoted )
                                CV_PARSE_ERROR( "Literal \" is not allowed within a string. Use &quot;" );
                            ++ptr;
                            break;
                        }
                        else if( !cv_isprint(c) || c == '<' || (!is_quoted && cv_isspace(c)))
                        {
                            if( is_quoted )
                                CV_PARSE_ERROR( "Closing \" is expected" );
                            break;
                        }
                        else if( c == '\'' || c == '>' )
                        {
                            CV_PARSE_ERROR( "Literal \' or > are not allowed. Use &apos; or &gt;" );
                        }
                        else if( c == '&' )
                        {
                            if( *++ptr == '#' )
                            {
                                int val, base = 10;
                                ptr++;
                                if( *ptr == 'x' )
                                {
                                    base = 16;
                                    ptr++;
                                }
                                val = (int)strtol( ptr, &endptr, base );
                                if( (unsigned)val > (unsigned)255 ||
                                    !endptr || *endptr != ';' )
                                    CV_PARSE_ERROR( "Invalid numeric value in the string" );
                                c = (char)val;
                            }
                            else
                            {
                                endptr = ptr;
                                do c = *++endptr;
                                while( cv_isalnum(c) );
                                if( c != ';' )
                                    CV_PARSE_ERROR( "Invalid character in the symbol entity name" );
                                len = (int)(endptr - ptr);
                                if( len == 2 && memcmp( ptr, "lt", len ) == 0 )
                                    c = '<';
                                else if( len == 2 && memcmp( ptr, "gt", len ) == 0 )
                                    c = '>';
                                else if( len == 3 && memcmp( ptr, "amp", len ) == 0 )
                                    c = '&';
                                else if( len == 4 && memcmp( ptr, "apos", len ) == 0 )
                                    c = '\'';
                                else if( len == 4 && memcmp( ptr, "quot", len ) == 0 )
                                    c = '\"';
                                else
                                {
                                    memcpy( buf + i, ptr-1, len + 2 );
                                    i += len + 2;
                                }
                            }
                            ptr = endptr;
                        }
                    }
                    buf[i++] = c;
                    if( i >= CV_FS_MAX_LEN )
                        CV_PARSE_ERROR( "Too long string literal" );
                }
                elem->data.str = cvMemStorageAllocString( fs->memstorage, buf, i );
            }

            if( !CV_NODE_IS_COLLECTION(value_type) && value_type != CV_NODE_NONE )
                break;
            have_space = false;
        }
    }

    if( (CV_NODE_TYPE(node->tag) == CV_NODE_NONE ||
        (CV_NODE_TYPE(node->tag) != value_type &&
        !CV_NODE_IS_COLLECTION(node->tag))) &&
        CV_NODE_IS_COLLECTION(value_type) )
    {
        icvFSCreateCollection( fs, CV_NODE_IS_MAP(value_type) ?
                                        CV_NODE_MAP : CV_NODE_SEQ, node );
    }

    if( value_type != CV_NODE_NONE &&
        value_type != CV_NODE_TYPE(node->tag) )
        CV_PARSE_ERROR( "The actual type is different from the specified type" );

    if( CV_NODE_IS_COLLECTION(node->tag) && is_simple )
            node->data.seq->flags |= CV_NODE_SEQ_SIMPLE;

    node->tag |= is_user_type ? CV_NODE_USER : 0;
    return ptr;
}


static char*
icvXMLParseTag( CvFileStorage* fs, char* ptr, CvStringHashNode** _tag,
                CvAttrList** _list, int* _tag_type )
{
    int tag_type = 0;
    CvStringHashNode* tagname = 0;
    CvAttrList *first = 0, *last = 0;
    int count = 0, max_count = 4;
    int attr_buf_size = (max_count*2 + 1)*sizeof(char*) + sizeof(CvAttrList);
    char* endptr;
    char c;
    int have_space;

    if( *ptr == '\0' )
        CV_PARSE_ERROR( "Preliminary end of the stream" );

    if( *ptr != '<' )
        CV_PARSE_ERROR( "Tag should start with \'<\'" );

    ptr++;
    if( cv_isalnum(*ptr) || *ptr == '_' )
        tag_type = CV_XML_OPENING_TAG;
    else if( *ptr == '/' )
    {
        tag_type = CV_XML_CLOSING_TAG;
        ptr++;
    }
    else if( *ptr == '?' )
    {
        tag_type = CV_XML_HEADER_TAG;
        ptr++;
    }
    else if( *ptr == '!' )
    {
        tag_type = CV_XML_DIRECTIVE_TAG;
        assert( ptr[1] != '-' || ptr[2] != '-' );
        ptr++;
    }
    else
        CV_PARSE_ERROR( "Unknown tag type" );

    for(;;)
    {
        CvStringHashNode* attrname;

        if( !cv_isalpha(*ptr) && *ptr != '_' )
            CV_PARSE_ERROR( "Name should start with a letter or underscore" );

        endptr = ptr - 1;
        do c = *++endptr;
        while( cv_isalnum(c) || c == '_' || c == '-' );

        attrname = cvGetHashedKey( fs, ptr, (int)(endptr - ptr), 1 );
        CV_Assert(attrname);
        ptr = endptr;

        if( !tagname )
            tagname = attrname;
        else
        {
            if( tag_type == CV_XML_CLOSING_TAG )
                CV_PARSE_ERROR( "Closing tag should not contain any attributes" );

            if( !last || count >= max_count )
            {
                CvAttrList* chunk;

                chunk = (CvAttrList*)cvMemStorageAlloc( fs->memstorage, attr_buf_size );
                memset( chunk, 0, attr_buf_size );
                chunk->attr = (const char**)(chunk + 1);
                count = 0;
                if( !last )
                    first = last = chunk;
                else
                    last = last->next = chunk;
            }
            last->attr[count*2] = attrname->str.ptr;
        }

        if( last )
        {
            CvFileNode stub;

            if( *ptr != '=' )
            {
                ptr = icvXMLSkipSpaces( fs, ptr, CV_XML_INSIDE_TAG );
                if( *ptr != '=' )
                    CV_PARSE_ERROR( "Attribute name should be followed by \'=\'" );
            }

            c = *++ptr;
            if( c != '\"' && c != '\'' )
            {
                ptr = icvXMLSkipSpaces( fs, ptr, CV_XML_INSIDE_TAG );
                if( *ptr != '\"' && *ptr != '\'' )
                    CV_PARSE_ERROR( "Attribute value should be put into single or double quotes" );
            }

            ptr = icvXMLParseValue( fs, ptr, &stub, CV_NODE_STRING );
            assert( stub.tag == CV_NODE_STRING );
            last->attr[count*2+1] = stub.data.str.ptr;
            count++;
        }

        c = *ptr;
        have_space = cv_isspace(c) || c == '\0';

        if( c != '>' )
        {
            ptr = icvXMLSkipSpaces( fs, ptr, CV_XML_INSIDE_TAG );
            c = *ptr;
        }

        if( c == '>' )
        {
            if( tag_type == CV_XML_HEADER_TAG )
                CV_PARSE_ERROR( "Invalid closing tag for <?xml ..." );
            ptr++;
            break;
        }
        else if( c == '?' && tag_type == CV_XML_HEADER_TAG )
        {
            if( ptr[1] != '>'  )
                CV_PARSE_ERROR( "Invalid closing tag for <?xml ..." );
            ptr += 2;
            break;
        }
        else if( c == '/' && ptr[1] == '>' && tag_type == CV_XML_OPENING_TAG )
        {
            tag_type = CV_XML_EMPTY_TAG;
            ptr += 2;
            break;
        }

        if( !have_space )
            CV_PARSE_ERROR( "There should be space between attributes" );
    }

    *_tag = tagname;
    *_tag_type = tag_type;
    *_list = first;

    return ptr;
}


void
icvXMLParse( CvFileStorage* fs )
{
    char* ptr = fs->buffer_start;
    CvStringHashNode *key = 0, *key2 = 0;
    CvAttrList* list = 0;
    int tag_type = 0;

    // CV_XML_INSIDE_TAG is used to prohibit leading comments
    ptr = icvXMLSkipSpaces( fs, ptr, CV_XML_INSIDE_TAG );

    if( memcmp( ptr, "<?xml", 5 ) != 0 )
        CV_PARSE_ERROR( "Valid XML should start with \'<?xml ...?>\'" );

    ptr = icvXMLParseTag( fs, ptr, &key, &list, &tag_type );

    while( *ptr != '\0' )
    {
        ptr = icvXMLSkipSpaces( fs, ptr, 0 );

        if( *ptr != '\0' )
        {
            CvFileNode* root_node;
            ptr = icvXMLParseTag( fs, ptr, &key, &list, &tag_type );
            if( tag_type != CV_XML_OPENING_TAG ||
                !key ||
                strcmp(key->str.ptr,"opencv_storage") != 0 )
                CV_PARSE_ERROR( "<opencv_storage> tag is missing" );

            root_node = (CvFileNode*)cvSeqPush( fs->roots, 0 );
            ptr = icvXMLParseValue( fs, ptr, root_node, CV_NODE_NONE );
            ptr = icvXMLParseTag( fs, ptr, &key2, &list, &tag_type );
            if( tag_type != CV_XML_CLOSING_TAG || key != key2 )
                CV_PARSE_ERROR( "</opencv_storage> tag is missing" );
            ptr = icvXMLSkipSpaces( fs, ptr, 0 );
        }
    }

    assert( fs->dummy_eof != 0 );
}


/****************************************************************************************\
*                                       XML Emitter                                      *
\****************************************************************************************/

#define icvXMLFlush icvFSFlush

static void
icvXMLWriteTag( CvFileStorage* fs, const char* key, int tag_type, CvAttrList list )
{
    char* ptr = fs->buffer;
    int i, len = 0;
    int struct_flags = fs->struct_flags;

    if( key && key[0] == '\0' )
        key = 0;

    if( tag_type == CV_XML_OPENING_TAG || tag_type == CV_XML_EMPTY_TAG )
    {
        if( CV_NODE_IS_COLLECTION(struct_flags) )
        {
            if( CV_NODE_IS_MAP(struct_flags) ^ (key != 0) )
                CV_Error( CV_StsBadArg, "An attempt to add element without a key to a map, "
                                        "or add element with key to sequence" );
        }
        else
        {
            struct_flags = CV_NODE_EMPTY + (key ? CV_NODE_MAP : CV_NODE_SEQ);
            fs->is_first = 0;
        }

        if( !CV_NODE_IS_EMPTY(struct_flags) )
            ptr = icvXMLFlush(fs);
    }

    if( !key )
        key = "_";
    else if( key[0] == '_' && key[1] == '\0' )
        CV_Error( CV_StsBadArg, "A single _ is a reserved tag name" );

    len = (int)strlen( key );
    *ptr++ = '<';
    if( tag_type == CV_XML_CLOSING_TAG )
    {
        if( list.attr )
            CV_Error( CV_StsBadArg, "Closing tag should not include any attributes" );
        *ptr++ = '/';
    }

    if( !cv_isalpha(key[0]) && key[0] != '_' )
        CV_Error( CV_StsBadArg, "Key should start with a letter or _" );

    ptr = icvFSResizeWriteBuffer( fs, ptr, len );
    for( i = 0; i < len; i++ )
    {
        char c = key[i];
        if( !cv_isalnum(c) && c != '_' && c != '-' )
            CV_Error( CV_StsBadArg, "Key name may only contain alphanumeric characters [a-zA-Z0-9], '-' and '_'" );
        ptr[i] = c;
    }
    ptr += len;

    for(;;)
    {
        const char** attr = list.attr;

        for( ; attr && attr[0] != 0; attr += 2 )
        {
            int len0 = (int)strlen(attr[0]);
            int len1 = (int)strlen(attr[1]);

            ptr = icvFSResizeWriteBuffer( fs, ptr, len0 + len1 + 4 );
            *ptr++ = ' ';
            memcpy( ptr, attr[0], len0 );
            ptr += len0;
            *ptr++ = '=';
            *ptr++ = '\"';
            memcpy( ptr, attr[1], len1 );
            ptr += len1;
            *ptr++ = '\"';
        }
        if( !list.next )
            break;
        list = *list.next;
    }

    if( tag_type == CV_XML_EMPTY_TAG )
        *ptr++ = '/';
    *ptr++ = '>';
    fs->buffer = ptr;
    fs->struct_flags = struct_flags & ~CV_NODE_EMPTY;
}
 
void
icvXMLStartWriteStruct( CvFileStorage* fs, const char* key, int struct_flags,
                        const char* type_name)
{
    CvXMLStackRecord parent;
    const char* attr[10];
    int idx = 0;

    struct_flags = (struct_flags & (CV_NODE_TYPE_MASK|CV_NODE_FLOW)) | CV_NODE_EMPTY;
    if( !CV_NODE_IS_COLLECTION(struct_flags))
        CV_Error( CV_StsBadArg,
        "Some collection type: CV_NODE_SEQ or CV_NODE_MAP must be specified" );

    if ( type_name && *type_name == '\0' )
        type_name = 0;

    if( type_name )
    {
        attr[idx++] = "type_id";
        attr[idx++] = type_name;
    }
    attr[idx++] = 0;

    icvXMLWriteTag( fs, key, CV_XML_OPENING_TAG, cvAttrList(attr,0) );

    parent.struct_flags = fs->struct_flags & ~CV_NODE_EMPTY;
    parent.struct_indent = fs->struct_indent;
    parent.struct_tag = fs->struct_tag;
    cvSaveMemStoragePos( fs->strstorage, &parent.pos );
    cvSeqPush( fs->write_stack, &parent );

    fs->struct_indent += CV_XML_INDENT;
    if( !CV_NODE_IS_FLOW(struct_flags) )
        icvXMLFlush( fs );

    fs->struct_flags = struct_flags;
    if( key )
    {
        fs->struct_tag = cvMemStorageAllocString( fs->strstorage, (char*)key, -1 );
    }
    else
    {
        fs->struct_tag.ptr = 0;
        fs->struct_tag.len = 0;
    }
}

void
icvXMLEndWriteStruct( CvFileStorage* fs )
{
    CvXMLStackRecord parent;

    if( fs->write_stack->total == 0 )
        CV_Error( CV_StsError, "An extra closing tag" );

    icvXMLWriteTag( fs, fs->struct_tag.ptr, CV_XML_CLOSING_TAG, cvAttrList(0,0) );
    cvSeqPop( fs->write_stack, &parent );

    fs->struct_indent = parent.struct_indent;
    fs->struct_flags = parent.struct_flags;
    fs->struct_tag = parent.struct_tag;
    cvRestoreMemStoragePos( fs->strstorage, &parent.pos );
}


void
icvXMLStartNextStream( CvFileStorage* fs )
{
    if( !fs->is_first )
    {
        while( fs->write_stack->total > 0 )
            icvXMLEndWriteStruct(fs);

        fs->struct_indent = 0;
        icvXMLFlush(fs);
        /* XML does not allow multiple top-level elements,
           so we just put a comment and continue
           the current (and the only) "stream" */
        icvPuts( fs, "\n<!-- next stream -->\n" );
        /*fputs( "</opencv_storage>\n", fs->file );
        fputs( "<opencv_storage>\n", fs->file );*/
        fs->buffer = fs->buffer_start;
    }
}

void
icvXMLWriteScalar( CvFileStorage* fs, const char* key, const char* data, int len )
{
    check_if_write_struct_is_delayed( fs );
    if ( fs->state_of_writing_base64 == base64::fs::Uncertain )
    {
        switch_to_Base64_state( fs, base64::fs::NotUse );
    }
    else if ( fs->state_of_writing_base64 == base64::fs::InUse )
    {
        CV_Error( CV_StsError, "Currently only Base64 data is allowed." );
    }

    if( CV_NODE_IS_MAP(fs->struct_flags) ||
        (!CV_NODE_IS_COLLECTION(fs->struct_flags) && key) )
    {
        icvXMLWriteTag( fs, key, CV_XML_OPENING_TAG, cvAttrList(0,0) );
        char* ptr = icvFSResizeWriteBuffer( fs, fs->buffer, len );
        memcpy( ptr, data, len );
        fs->buffer = ptr + len;
        icvXMLWriteTag( fs, key, CV_XML_CLOSING_TAG, cvAttrList(0,0) );
    }
    else
    {
        char* ptr = fs->buffer;
        int new_offset = (int)(ptr - fs->buffer_start) + len;

        if( key )
            CV_Error( CV_StsBadArg, "elements with keys can not be written to sequence" );

        fs->struct_flags = CV_NODE_SEQ;

        if( (new_offset > fs->wrap_margin && new_offset - fs->struct_indent > 10) ||
            (ptr > fs->buffer_start && ptr[-1] == '>' && !CV_NODE_IS_EMPTY(fs->struct_flags)) )
        {
            ptr = icvXMLFlush(fs);
        }
        else if( ptr > fs->buffer_start + fs->struct_indent && ptr[-1] != '>' )
            *ptr++ = ' ';

        memcpy( ptr, data, len );
        fs->buffer = ptr + len;
    }
}
 
void
icvXMLWriteInt( CvFileStorage* fs, const char* key, int value )
{
    char buf[128], *ptr = icv_itoa( value, buf, 10 );
    int len = (int)strlen(ptr);
    icvXMLWriteScalar( fs, key, ptr, len );
}
 
void
icvXMLWriteReal( CvFileStorage* fs, const char* key, double value )
{
    char buf[128];
    int len = (int)strlen( icvDoubleToString( buf, value ));
    icvXMLWriteScalar( fs, key, buf, len );
}

void
icvXMLWriteString( CvFileStorage* fs, const char* key, const char* str, int quote )
{
    char buf[CV_FS_MAX_LEN*6+16];
    char* data = (char*)str;
    int i, len;

    if( !str )
        CV_Error( CV_StsNullPtr, "Null string pointer" );

    len = (int)strlen(str);
    if( len > CV_FS_MAX_LEN )
        CV_Error( CV_StsBadArg, "The written string is too long" );

    if( quote || len == 0 || str[0] != '\"' || str[0] != str[len-1] )
    {
        int need_quote = quote || len == 0;
        data = buf;
        *data++ = '\"';
        for( i = 0; i < len; i++ )
        {
            char c = str[i];

            if( (uchar)c >= 128 || c == ' ' )
            {
                *data++ = c;
                need_quote = 1;
            }
            else if( !cv_isprint(c) || c == '<' || c == '>' || c == '&' || c == '\'' || c == '\"' )
            {
                *data++ = '&';
                if( c == '<' )
                {
                    memcpy(data, "lt", 2);
                    data += 2;
                }
                else if( c == '>' )
                {
                    memcpy(data, "gt", 2);
                    data += 2;
                }
                else if( c == '&' )
                {
                    memcpy(data, "amp", 3);
                    data += 3;
                }
                else if( c == '\'' )
                {
                    memcpy(data, "apos", 4);
                    data += 4;
                }
                else if( c == '\"' )
                {
                    memcpy( data, "quot", 4);
                    data += 4;
                }
                else
                {
                    sprintf( data, "#x%02x", (uchar)c );
                    data += 4;
                }
                *data++ = ';';
                need_quote = 1;
            }
            else
                *data++ = c;
        }
        if( !need_quote && (cv_isdigit(str[0]) ||
            str[0] == '+' || str[0] == '-' || str[0] == '.' ))
            need_quote = 1;

        if( need_quote )
            *data++ = '\"';
        len = (int)(data - buf) - !need_quote;
        *data++ = '\0';
        data = buf + !need_quote;
    }

    icvXMLWriteScalar( fs, key, data, len );
}


void
icvXMLWriteComment( CvFileStorage* fs, const char* comment, int eol_comment )
{
    int len;
    int multiline;
    const char* eol;
    char* ptr;

    if( !comment )
        CV_Error( CV_StsNullPtr, "Null comment" );

    if( strstr(comment, "--") != 0 )
        CV_Error( CV_StsBadArg, "Double hyphen \'--\' is not allowed in the comments" );

    len = (int)strlen(comment);
    eol = strchr(comment, '\n');
    multiline = eol != 0;
    ptr = fs->buffer;

    if( multiline || !eol_comment || fs->buffer_end - ptr < len + 5 )
        ptr = icvXMLFlush( fs );
    else if( ptr > fs->buffer_start + fs->struct_indent )
        *ptr++ = ' ';

    if( !multiline )
    {
        ptr = icvFSResizeWriteBuffer( fs, ptr, len + 9 );
        sprintf( ptr, "<!-- %s -->", comment );
        len = (int)strlen(ptr);
    }
    else
    {
        strcpy( ptr, "<!--" );
        len = 4;
    }

    fs->buffer = ptr + len;
    ptr = icvXMLFlush(fs);

    if( multiline )
    {
        while( comment )
        {
            if( eol )
            {
                ptr = icvFSResizeWriteBuffer( fs, ptr, (int)(eol - comment) + 1 );
                memcpy( ptr, comment, eol - comment + 1 );
                ptr += eol - comment;
                comment = eol + 1;
                eol = strchr( comment, '\n' );
            }
            else
            {
                len = (int)strlen(comment);
                ptr = icvFSResizeWriteBuffer( fs, ptr, len );
                memcpy( ptr, comment, len );
                ptr += len;
                comment = 0;
            }
            fs->buffer = ptr;
            ptr = icvXMLFlush( fs );
        }
        sprintf( ptr, "-->" );
        fs->buffer = ptr + 3;
        icvXMLFlush( fs );
    }
}


/****************************************************************************************\
*                              Common High-Level Functions                               *
\****************************************************************************************/

CV_IMPL CvFileStorage*
cvOpenFileStorage( const char* query, CvMemStorage* dststorage, int flags, const char* encoding )
{
    CvFileStorage* fs = 0;
    int default_block_size = 1 << 18;
    bool append = (flags & 3) == CV_STORAGE_APPEND;
    bool mem = (flags & CV_STORAGE_MEMORY) != 0;
    bool write_mode = (flags & 3) != 0;
    bool write_base64 = (write_mode || append) && (flags & CV_STORAGE_BASE64) != 0;
    bool isGZ = false;
    size_t fnamelen = 0;
    const char * filename = query;

    std::vector<std::string> params;
    if ( !mem )
    {
        params = analyze_file_name( query );
        if ( !params.empty() )
            filename = params.begin()->c_str();

        if ( write_base64 == false && is_param_exist( params, "base64" ) )
            write_base64 = (write_mode || append);
    }

    if( !filename || filename[0] == '\0' )
    {
        if( !write_mode )
            CV_Error( CV_StsNullPtr, mem ? "NULL or empty filename" : "NULL or empty buffer" );
        mem = true;
    }
    else
        fnamelen = strlen(filename);

    if( mem && append )
        CV_Error( CV_StsBadFlag, "CV_STORAGE_APPEND and CV_STORAGE_MEMORY are not currently compatible" );

    fs = (CvFileStorage*)cvAlloc( sizeof(*fs) );
    CV_Assert(fs);
    memset( fs, 0, sizeof(*fs));

    fs->memstorage = cvCreateMemStorage( default_block_size );
    fs->dststorage = dststorage ? dststorage : fs->memstorage;

    fs->flags = CV_FILE_STORAGE;
    fs->write_mode = write_mode;

    if( !mem )
    {
        fs->filename = (char*)cvMemStorageAlloc( fs->memstorage, fnamelen+1 );
        strcpy( fs->filename, filename );

        char* dot_pos = strrchr(fs->filename, '.');
        char compression = '\0';

        if( dot_pos && dot_pos[1] == 'g' && dot_pos[2] == 'z' &&
            (dot_pos[3] == '\0' || (cv_isdigit(dot_pos[3]) && dot_pos[4] == '\0')) )
        {
            if( append )
            {
                cvReleaseFileStorage( &fs );
                CV_Error(CV_StsNotImplemented, "Appending data to compressed file is not implemented" );
            }
            isGZ = true;
            compression = dot_pos[3];
            if( compression )
                dot_pos[3] = '\0', fnamelen--;
        }

        if( !isGZ )
        {
            fs->file = fopen(fs->filename, !fs->write_mode ? "rt" : !append ? "wt" : "a+t" );
            if( !fs->file )
                goto _exit_;
        }
        else
        {
            #if USE_ZLIB
            char mode[] = { fs->write_mode ? 'w' : 'r', 'b', compression ? compression : '3', '\0' };
            fs->gzfile = gzopen(fs->filename, mode);
            if( !fs->gzfile )
                goto _exit_;
            #else
            cvReleaseFileStorage( &fs );
            CV_Error(CV_StsNotImplemented, "There is no compressed file storage support in this configuration");
            #endif
        }
    }

    fs->roots = 0;
    fs->struct_indent = 0;
    fs->struct_flags = 0;
    fs->wrap_margin = 71;

    if( fs->write_mode )
    {
        int fmt = flags & CV_STORAGE_FORMAT_MASK;

        if( mem )
            fs->outbuf = new std::deque<char>;

        if( fmt == CV_STORAGE_FORMAT_AUTO && filename )
        {
            const char* dot_pos = NULL;
            const char* dot_pos2 = NULL;
            // like strrchr() implementation, but save two last positions simultaneously
            for (const char* pos = filename; pos[0] != 0; pos++)
            {
                if (pos[0] == '.')
                {
                    dot_pos2 = dot_pos;
                    dot_pos = pos;
                }
            }
            if (cv_strcasecmp(dot_pos, ".gz") && dot_pos2 != NULL)
            {
                dot_pos = dot_pos2;
            }
            fs->fmt
                = (cv_strcasecmp(dot_pos, ".xml") || cv_strcasecmp(dot_pos, ".xml.gz"))
                ? CV_STORAGE_FORMAT_XML
                : (cv_strcasecmp(dot_pos, ".json") || cv_strcasecmp(dot_pos, ".json.gz"))
                ? CV_STORAGE_FORMAT_JSON
                : CV_STORAGE_FORMAT_YAML
                ;
        }
        else if ( fmt != CV_STORAGE_FORMAT_AUTO )
        {
            fs->fmt = fmt;
        }
        else
        {
            fs->fmt = CV_STORAGE_FORMAT_XML;
        }

        // we use factor=6 for XML (the longest characters (' and ") are encoded with 6 bytes (&apos; and &quot;)
        // and factor=4 for YAML ( as we use 4 bytes for non ASCII characters (e.g. \xAB))
        int buf_size = CV_FS_MAX_LEN*(fs->fmt == CV_STORAGE_FORMAT_XML ? 6 : 4) + 1024;

        if( append )
            fseek( fs->file, 0, SEEK_END );

        fs->write_stack = cvCreateSeq( 0, sizeof(CvSeq), fs->fmt == CV_STORAGE_FORMAT_XML ?
                sizeof(CvXMLStackRecord) : sizeof(int), fs->memstorage );
        fs->is_first = 1;
        fs->struct_indent = 0;
        fs->struct_flags = CV_NODE_EMPTY;
        fs->buffer_start = fs->buffer = (char*)cvAlloc( buf_size + 1024 );
        fs->buffer_end = fs->buffer_start + buf_size;

        fs->base64_writer           = 0;
        fs->is_default_using_base64 = write_base64;
        fs->state_of_writing_base64 = base64::fs::Uncertain;

        fs->is_write_struct_delayed = false;
        fs->delayed_struct_key      = 0;
        fs->delayed_struct_flags    = 0;
        fs->delayed_type_name       = 0;

        if( fs->fmt == CV_STORAGE_FORMAT_XML )
        {
            size_t file_size = fs->file ? (size_t)ftell( fs->file ) : (size_t)0;
            fs->strstorage = cvCreateChildMemStorage( fs->memstorage );
            if( !append || file_size == 0 )
            {
                if( encoding )
                {
                    if( strcmp( encoding, "UTF-16" ) == 0 ||
                        strcmp( encoding, "utf-16" ) == 0 ||
                        strcmp( encoding, "Utf-16" ) == 0 )
                    {
                        cvReleaseFileStorage( &fs );
                        CV_Error( CV_StsBadArg, "UTF-16 XML encoding is not supported! Use 8-bit encoding\n");
                    }

                    CV_Assert( strlen(encoding) < 1000 );
                    char buf[1100];
                    sprintf(buf, "<?xml version=\"1.0\" encoding=\"%s\"?>\n", encoding);
                    icvPuts( fs, buf );
                }
                else
                    icvPuts( fs, "<?xml version=\"1.0\"?>\n" );
                icvPuts( fs, "<opencv_storage>\n" );
            }
            else
            {
                int xml_buf_size = 1 << 10;
                char substr[] = "</opencv_storage>";
                int last_occurence = -1;
                xml_buf_size = MIN(xml_buf_size, int(file_size));
                fseek( fs->file, -xml_buf_size, SEEK_END );
                char* xml_buf = (char*)cvAlloc( xml_buf_size+2 );
                // find the last occurence of </opencv_storage>
                for(;;)
                {
                    int line_offset = (int)ftell( fs->file );
                    char* ptr0 = icvGets( fs, xml_buf, xml_buf_size ), *ptr;
                    if( !ptr0 )
                        break;
                    ptr = ptr0;
                    for(;;)
                    {
                        ptr = strstr( ptr, substr );
                        if( !ptr )
                            break;
                        last_occurence = line_offset + (int)(ptr - ptr0);
                        ptr += strlen(substr);
                    }
                }
                cvFree( &xml_buf );
                if( last_occurence < 0 )
                {
                    cvReleaseFileStorage( &fs );
                    CV_Error( CV_StsError, "Could not find </opencv_storage> in the end of file.\n" );
                }
                icvCloseFile( fs );
                fs->file = fopen( fs->filename, "r+t" );
                CV_Assert(fs->file);
                fseek( fs->file, last_occurence, SEEK_SET );
                // replace the last "</opencv_storage>" with " <!-- resumed -->", which has the same length
                icvPuts( fs, " <!-- resumed -->" );
                fseek( fs->file, 0, SEEK_END );
                icvPuts( fs, "\n" );
            }
            fs->start_write_struct = icvXMLStartWriteStruct;
            fs->end_write_struct = icvXMLEndWriteStruct;
            fs->write_int = icvXMLWriteInt;
            fs->write_real = icvXMLWriteReal;
            fs->write_string = icvXMLWriteString;
            fs->write_comment = icvXMLWriteComment;
            fs->start_next_stream = icvXMLStartNextStream;
        }
//         else if( fs->fmt == CV_STORAGE_FORMAT_YAML )
//         {
//             if( !append )
//                 icvPuts( fs, "%YAML:1.0\n---\n" );
//             else
//                 icvPuts( fs, "...\n---\n" );
//             fs->start_write_struct = icvYMLStartWriteStruct;
//             fs->end_write_struct = icvYMLEndWriteStruct;
//             fs->write_int = icvYMLWriteInt;
//             fs->write_real = icvYMLWriteReal;
//             fs->write_string = icvYMLWriteString;
//             fs->write_comment = icvYMLWriteComment;
//             fs->start_next_stream = icvYMLStartNextStream;
//         }
        else
        {
            if( !append )
                icvPuts( fs, "{\n" );
            else
            {
                bool valid = false;
                long roffset = 0;
                for ( ;
                      fseek( fs->file, roffset, SEEK_END ) == 0;
                      roffset -= 1 )
                {
                    const char end_mark = '}';
                    if ( fgetc( fs->file ) == end_mark )
                    {
                        fseek( fs->file, roffset, SEEK_END );
                        valid = true;
                        break;
                    }
                }

                if ( valid )
                {
                    icvCloseFile( fs );
                    fs->file = fopen( fs->filename, "r+t" );
                    CV_Assert(fs->file);
                    fseek( fs->file, roffset, SEEK_END );
                    fputs( ",", fs->file );
                }
                else
                {
                    CV_Error( CV_StsError, "Could not find '}' in the end of file.\n" );
                }
            }
//             fs->struct_indent = 4;
//             fs->start_write_struct = icvJSONStartWriteStruct;
//             fs->end_write_struct = icvJSONEndWriteStruct;
//             fs->write_int = icvJSONWriteInt;
//             fs->write_real = icvJSONWriteReal;
//             fs->write_string = icvJSONWriteString;
//             fs->write_comment = icvJSONWriteComment;
//             fs->start_next_stream = icvJSONStartNextStream;
        }
    }
    else
    {
        if( mem )
        {
            fs->strbuf = filename;
            fs->strbufsize = fnamelen;
        }

        size_t buf_size = 1 << 20;
        const char* yaml_signature = "%YAML";
        const char* json_signature = "{";
        const char* xml_signature  = "<?xml";
        char buf[16];
        icvGets( fs, buf, sizeof(buf)-2 );
        char* bufPtr = cv_skip_BOM(buf);
        size_t bufOffset = bufPtr - buf;

        if(strncmp( bufPtr, yaml_signature, strlen(yaml_signature) ) == 0)
            fs->fmt = CV_STORAGE_FORMAT_YAML;
        else if(strncmp( bufPtr, json_signature, strlen(json_signature) ) == 0)
            fs->fmt = CV_STORAGE_FORMAT_JSON;
        else if(strncmp( bufPtr, xml_signature, strlen(xml_signature) ) == 0)
            fs->fmt = CV_STORAGE_FORMAT_XML;
        else if(fs->strbufsize  == bufOffset)
            CV_Error(CV_BADARG_ERR, "Input file is empty");
        else
            CV_Error(CV_BADARG_ERR, "Unsupported file storage format");

        if( !isGZ )
        {
            if( !mem )
            {
                fseek( fs->file, 0, SEEK_END );
                buf_size = ftell( fs->file );
            }
            else
                buf_size = fs->strbufsize;
            buf_size = MIN( buf_size, (size_t)(1 << 20) );
            buf_size = MAX( buf_size, (size_t)(CV_FS_MAX_LEN*2 + 1024) );
        }
        icvRewind(fs);
        fs->strbufpos = bufOffset;

        fs->str_hash = cvCreateMap( 0, sizeof(CvStringHash),
                        sizeof(CvStringHashNode), fs->memstorage, 256 );

        fs->roots = cvCreateSeq( 0, sizeof(CvSeq),
                        sizeof(CvFileNode), fs->memstorage );

        fs->buffer = fs->buffer_start = (char*)cvAlloc( buf_size + 256 );
        fs->buffer_end = fs->buffer_start + buf_size;
        fs->buffer[0] = '\n';
        fs->buffer[1] = '\0';

        //mode = cvGetErrMode();
        //cvSetErrMode( CV_ErrModeSilent );
        CV_TRY
        {
            switch (fs->fmt)
            {
            case CV_STORAGE_FORMAT_XML : { icvXMLParse ( fs ); break; }
//             case CV_STORAGE_FORMAT_YAML: { icvYMLParse ( fs ); break; }
//             case CV_STORAGE_FORMAT_JSON: { icvJSONParse( fs ); break; }
            default: break;
            }
        }
        CV_CATCH_ALL
        {
            fs->is_opened = true;
            cvReleaseFileStorage( &fs );
            CV_RETHROW();
        }
        //cvSetErrMode( mode );

        // release resources that we do not need anymore
        cvFree( &fs->buffer_start );
        fs->buffer = fs->buffer_end = 0;
    }
    fs->is_opened = true;

_exit_:
    if( fs )
    {
        if( cvGetErrStatus() < 0 || (!fs->file && !fs->gzfile && !fs->outbuf && !fs->strbuf) )
        {
            cvReleaseFileStorage( &fs );
        }
        else if( !fs->write_mode )
        {
            icvCloseFile(fs);
            // we close the file since it's not needed anymore. But icvCloseFile() resets is_opened,
            // which may be misleading. Since we restore the value of is_opened.
            fs->is_opened = true;
        }
    }

    return  fs;
}

CV_IMPL void
cvStartWriteStruct( CvFileStorage* fs, const char* key, int struct_flags,
                    const char* type_name, CvAttrList /*attributes*/ )
{
    CV_CHECK_OUTPUT_FILE_STORAGE(fs);
    check_if_write_struct_is_delayed( fs );
    if ( fs->state_of_writing_base64 == base64::fs::NotUse )
        switch_to_Base64_state( fs, base64::fs::Uncertain );

    if ( fs->state_of_writing_base64 == base64::fs::Uncertain
        &&
        CV_NODE_IS_SEQ(struct_flags)
        &&
        fs->is_default_using_base64
        &&
        type_name == 0
       )
    {
        /* Uncertain whether output Base64 data */
//         make_write_struct_delayed( fs, key, struct_flags, type_name );
    }
    else if ( type_name && memcmp(type_name, "binary", 6) == 0 )
    {
        /* Must output Base64 data */
        if ( !CV_NODE_IS_SEQ(struct_flags) )
            CV_Error( CV_StsBadArg, "must set 'struct_flags |= CV_NODE_SEQ' if using Base64.");
        else if ( fs->state_of_writing_base64 != base64::fs::Uncertain )
            CV_Error( CV_StsError, "function \'cvStartWriteStruct\' calls cannot be nested if using Base64.");

        fs->start_write_struct( fs, key, struct_flags, type_name );

        if ( fs->state_of_writing_base64 != base64::fs::Uncertain )
            switch_to_Base64_state( fs, base64::fs::Uncertain );
        switch_to_Base64_state( fs, base64::fs::InUse );
    }
    else
    {
        /* Won't output Base64 data */
        if ( fs->state_of_writing_base64 == base64::fs::InUse )
            CV_Error( CV_StsError, "At the end of the output Base64, `cvEndWriteStruct` is needed.");

        fs->start_write_struct( fs, key, struct_flags, type_name );

        if ( fs->state_of_writing_base64 != base64::fs::Uncertain )
            switch_to_Base64_state( fs, base64::fs::Uncertain );
        switch_to_Base64_state( fs, base64::fs::NotUse );
    }
}


CV_IMPL void
cvEndWriteStruct( CvFileStorage* fs )
{
    CV_CHECK_OUTPUT_FILE_STORAGE(fs);
    check_if_write_struct_is_delayed( fs );

    if ( fs->state_of_writing_base64 != base64::fs::Uncertain )
        switch_to_Base64_state( fs, base64::fs::Uncertain );

    fs->end_write_struct( fs );
}


CV_IMPL void
cvWriteInt( CvFileStorage* fs, const char* key, int value )
{
    CV_CHECK_OUTPUT_FILE_STORAGE(fs);
    fs->write_int( fs, key, value );
}


CV_IMPL void
cvWriteReal( CvFileStorage* fs, const char* key, double value )
{
    CV_CHECK_OUTPUT_FILE_STORAGE(fs);
    fs->write_real( fs, key, value );
}


CV_IMPL void
cvWriteString( CvFileStorage* fs, const char* key, const char* value, int quote )
{
    CV_CHECK_OUTPUT_FILE_STORAGE(fs);
    fs->write_string( fs, key, value, quote );
}


CV_IMPL void
cvWriteComment( CvFileStorage* fs, const char* comment, int eol_comment )
{
    CV_CHECK_OUTPUT_FILE_STORAGE(fs);
    fs->write_comment( fs, comment, eol_comment );
}

static inline int cvAlign( int size, int align )
{
    CV_DbgAssert( (align & (align-1)) == 0 && size < INT_MAX );
    return (size + align - 1) & -align;
}


static const char icvTypeSymbol[] = "ucwsifdr";
#define CV_FS_MAX_FMT_PAIRS  128

static int
icvDecodeFormat( const char* dt, int* fmt_pairs, int max_len )
{
    int fmt_pair_count = 0;
    int i = 0, k = 0, len = dt ? (int)strlen(dt) : 0;

    if( !dt || !len )
        return 0;

    assert( fmt_pairs != 0 && max_len > 0 );
    fmt_pairs[0] = 0;
    max_len *= 2;

    for( ; k < len; k++ )
    {
        char c = dt[k];

        if( cv_isdigit(c) )
        {
            int count = c - '0';
            if( cv_isdigit(dt[k+1]) )
            {
                char* endptr = 0;
                count = (int)strtol( dt+k, &endptr, 10 );
                k = (int)(endptr - dt) - 1;
            }

            if( count <= 0 )
                CV_Error( CV_StsBadArg, "Invalid data type specification" );

            fmt_pairs[i] = count;
        }
        else
        {
            const char* pos = strchr( icvTypeSymbol, c );
            if( !pos )
                CV_Error( CV_StsBadArg, "Invalid data type specification" );
            if( fmt_pairs[i] == 0 )
                fmt_pairs[i] = 1;
            fmt_pairs[i+1] = (int)(pos - icvTypeSymbol);
            if( i > 0 && fmt_pairs[i+1] == fmt_pairs[i-1] )
                fmt_pairs[i-2] += fmt_pairs[i];
            else
            {
                i += 2;
                if( i >= max_len )
                    CV_Error( CV_StsBadArg, "Too long data type specification" );
            }
            fmt_pairs[i] = 0;
        }
    }

    fmt_pair_count = i/2;
    return fmt_pair_count;
}


static int
icvCalcElemSize( const char* dt, int initial_size )
{
    int size = 0;
    int fmt_pairs[CV_FS_MAX_FMT_PAIRS], i, fmt_pair_count;
    int comp_size;

    fmt_pair_count = icvDecodeFormat( dt, fmt_pairs, CV_FS_MAX_FMT_PAIRS );
    fmt_pair_count *= 2;
    for( i = 0, size = initial_size; i < fmt_pair_count; i += 2 )
    {
        comp_size = CV_ELEM_SIZE(fmt_pairs[i+1]);
        size = cvAlign( size, comp_size );
        size += comp_size * fmt_pairs[i];
    }
    if( initial_size == 0 )
    {
        comp_size = CV_ELEM_SIZE(fmt_pairs[1]);
        size = cvAlign( size, comp_size );
    }
    return size;
}
// 

static int
icvCalcStructSize( const char* dt, int initial_size )
{
    int size = icvCalcElemSize( dt, initial_size );
    size_t elem_max_size = 0;
    for ( const char * type = dt; *type != '\0'; type++ ) {
        switch ( *type )
        {
        case 'u': { elem_max_size = std::max( elem_max_size, sizeof(uchar ) ); break; }
        case 'c': { elem_max_size = std::max( elem_max_size, sizeof(schar ) ); break; }
        case 'w': { elem_max_size = std::max( elem_max_size, sizeof(ushort) ); break; }
        case 's': { elem_max_size = std::max( elem_max_size, sizeof(short ) ); break; }
        case 'i': { elem_max_size = std::max( elem_max_size, sizeof(int   ) ); break; }
        case 'f': { elem_max_size = std::max( elem_max_size, sizeof(float ) ); break; }
        case 'd': { elem_max_size = std::max( elem_max_size, sizeof(double) ); break; }
        default: break;
        }
    }
    size = cvAlign( size, static_cast<int>(elem_max_size) );
    return size;
}


static void
icvWriteFileNode( CvFileStorage* fs, const char* name, const CvFileNode* node );


CV_IMPL const char*
cvGetFileNodeName( const CvFileNode* file_node )
{
    return file_node && CV_NODE_HAS_NAME(file_node->tag) ?
        ((CvFileMapNode*)file_node)->key->str.ptr : 0;
}

/****************************************************************************************\
*                                    RTTI Functions                                      *
\****************************************************************************************/

CvTypeInfo *CvType::first = 0, *CvType::last = 0;

CvType::CvType( const char* type_name,
                CvIsInstanceFunc is_instance, CvReleaseFunc release,
                CvReadFunc read, CvWriteFunc write, CvCloneFunc clone )
{
    CvTypeInfo _info;
    _info.flags = 0;
    _info.header_size = sizeof(_info);
    _info.type_name = type_name;
    _info.prev = _info.next = 0;
    _info.is_instance = is_instance;
    _info.release = release;
    _info.clone = clone;
    _info.read = read;
    _info.write = write;

    cvRegisterType( &_info );
    info = first;
}


CvType::~CvType()
{
    cvUnregisterType( info->type_name );
}

CV_IMPL  void
cvRegisterType( const CvTypeInfo* _info )
{
    CvTypeInfo* info = 0;
    int i, len;
    char c;

    //if( !CvType::first )
    //    icvCreateStandardTypes();

    if( !_info || _info->header_size != sizeof(CvTypeInfo) )
        CV_Error( CV_StsBadSize, "Invalid type info" );

    if( !_info->is_instance || !_info->release ||
        !_info->read || !_info->write )
        CV_Error( CV_StsNullPtr,
        "Some of required function pointers "
        "(is_instance, release, read or write) are NULL");

    c = _info->type_name[0];
    if( !cv_isalpha(c) && c != '_' )
        CV_Error( CV_StsBadArg, "Type name should start with a letter or _" );

    len = (int)strlen(_info->type_name);

    for( i = 0; i < len; i++ )
    {
        c = _info->type_name[i];
        if( !cv_isalnum(c) && c != '-' && c != '_' )
            CV_Error( CV_StsBadArg,
            "Type name should contain only letters, digits, - and _" );
    }

    info = (CvTypeInfo*)cvAlloc( sizeof(*info) + len + 1 );

    *info = *_info;
    info->type_name = (char*)(info + 1);
    memcpy( (char*)info->type_name, _info->type_name, len + 1 );

    info->flags = 0;
    info->next = CvType::first;
    info->prev = 0;
    if( CvType::first )
        CvType::first->prev = info;
    else
        CvType::last = info;
    CvType::first = info;
}


CV_IMPL void
cvUnregisterType( const char* type_name )
{
    CvTypeInfo* info;

    info = cvFindType( type_name );
    if( info )
    {
        if( info->prev )
            info->prev->next = info->next;
        else
            CvType::first = info->next;

        if( info->next )
            info->next->prev = info->prev;
        else
            CvType::last = info->prev;

        if( !CvType::first || !CvType::last )
            CvType::first = CvType::last = 0;

        cvFree( &info );
    }
}


CV_IMPL CvTypeInfo*
cvFindType( const char* type_name )
{
    CvTypeInfo* info = 0;

    if (type_name)
      for( info = CvType::first; info != 0; info = info->next )
        if( strcmp( info->type_name, type_name ) == 0 )
      break;

    return info;
}


CV_IMPL CvTypeInfo*
cvTypeOf( const void* struct_ptr )
{
    CvTypeInfo* info = 0;

    if( struct_ptr )
    {
        for( info = CvType::first; info != 0; info = info->next )
            if( info->is_instance( struct_ptr ))
                break;
    }

    return info;
}

/* reads matrix, image, sequence, graph etc. */
CV_IMPL void*
cvRead( CvFileStorage* fs, CvFileNode* node, CvAttrList* list )
{
    void* obj = 0;
    CV_CHECK_FILE_STORAGE( fs );

    if( !node )
        return 0;

    if( !CV_NODE_IS_USER(node->tag) || !node->info )
        CV_Error( CV_StsError, "The node does not represent a user object (unknown type?)" );

    obj = node->info->read( fs, node );
    if( list )
        *list = cvAttrList(0,0);

    return obj;
}


/* writes matrix, image, sequence, graph etc. */
CV_IMPL void
cvWrite( CvFileStorage* fs, const char* name,
         const void* ptr, CvAttrList attributes )
{
    CvTypeInfo* info;

    CV_CHECK_OUTPUT_FILE_STORAGE( fs );

    if( !ptr )
        CV_Error( CV_StsNullPtr, "Null pointer to the written object" );

    info = cvTypeOf( ptr );
    if( !info )
        CV_Error( CV_StsBadArg, "Unknown object" );

    if( !info->write )
        CV_Error( CV_StsBadArg, "The object does not have write function" );

    info->write( fs, name, ptr, attributes );
}



CV_IMPL void*
cvLoad( const char* filename, CvMemStorage* memstorage,
        const char* name, const char** _real_name )
{
    void* ptr = 0;
    const char* real_name = 0;
    cv::FileStorage fs(cvOpenFileStorage(filename, memstorage, CV_STORAGE_READ));

    CvFileNode* node = 0;

    if( !fs.isOpened() )
        return 0;

    if( name )
    {
        node = cvGetFileNodeByName( *fs, 0, name );
    }
    else
    {
        int i, k;
        for( k = 0; k < (*fs)->roots->total; k++ )
        {
            CvSeq* seq;
            CvSeqReader reader;

            node = (CvFileNode*)cvGetSeqElem( (*fs)->roots, k );
            CV_Assert(node != NULL);
            if( !CV_NODE_IS_MAP( node->tag ))
                return 0;
            seq = node->data.seq;
            node = 0;

            cvStartReadSeq( seq, &reader, 0 );

            // find the first element in the map
            for( i = 0; i < seq->total; i++ )
            {
                if( CV_IS_SET_ELEM( reader.ptr ))
                {
                    node = (CvFileNode*)reader.ptr;
                    goto stop_search;
                }
                CV_NEXT_SEQ_ELEM( seq->elem_size, reader );
            }
        }

stop_search:
        ;
    }

    if( !node )
        CV_Error( CV_StsObjectNotFound, "Could not find the/an object in file storage" );

    real_name = cvGetFileNodeName( node );
    ptr = cvRead( *fs, node, 0 );

    // sanity check
    if( !memstorage && (CV_IS_SEQ( ptr ) || CV_IS_SET( ptr )) )
        CV_Error( CV_StsNullPtr,
        "NULL memory storage is passed - the loaded dynamic structure can not be stored" );

    if( cvGetErrStatus() < 0 )
    {
        cvRelease( (void**)&ptr );
        real_name = 0;
    }

    if( _real_name)
    {
    if (real_name)
    {
        *_real_name = (const char*)cvAlloc(strlen(real_name));
            memcpy((void*)*_real_name, real_name, strlen(real_name));
    } else {
        *_real_name = 0;
    }
    }

    return ptr;
}

#include <opencv2/core/utils/trace.hpp>

#define CV_INSTRUMENT_REGION_()                            CV_TRACE_FUNCTION()

#ifdef __CV_AVX_GUARD
#define CV_INSTRUMENT_REGION() __CV_AVX_GUARD CV_INSTRUMENT_REGION_()
#else
#define CV_INSTRUMENT_REGION() CV_INSTRUMENT_REGION_()
#endif

///////////////////////// new C++ interface for CvFileStorage ///////////////////////////
namespace cv
{

FileStorage::FileStorage()
{
    state = UNDEFINED;
}

FileStorage::FileStorage(const String& filename, int flags, const String& encoding)
{
    state = UNDEFINED;
    open( filename, flags, encoding );
}
FileStorage::FileStorage(CvFileStorage* _fs, bool owning)
{
    if (owning) fs.reset(_fs);
    else fs = Ptr<CvFileStorage>(Ptr<CvFileStorage>(), _fs);

    state = _fs ? NAME_EXPECTED + INSIDE_MAP : UNDEFINED;
}


FileStorage::~FileStorage()
{
    while( structs.size() > 0 )
    {
        cvEndWriteStruct(fs);
        structs.pop_back();
    }
}

bool FileStorage::open(const String& filename, int flags, const String& encoding)
{
    CV_INSTRUMENT_REGION()

    release();
    fs.reset(cvOpenFileStorage( filename.c_str(), 0, flags,
                                !encoding.empty() ? encoding.c_str() : 0));
    bool ok = isOpened();
    state = ok ? NAME_EXPECTED + INSIDE_MAP : UNDEFINED;
    return ok;
}

bool FileStorage::isOpened() const
{
    return fs && fs->is_opened;
}

void FileStorage::release()
{
    fs.release();
    structs.clear();
    state = UNDEFINED;
}

FileNode FileStorage::root(int streamidx) const
{
    return isOpened() ? FileNode(fs, cvGetRootFileNode(fs, streamidx)) : FileNode();
}


FileStorage& operator << (FileStorage& fs, const String& str)
{
    CV_TRACE_REGION_VERBOSE();

    enum { NAME_EXPECTED = FileStorage::NAME_EXPECTED,
        VALUE_EXPECTED = FileStorage::VALUE_EXPECTED,
        INSIDE_MAP = FileStorage::INSIDE_MAP };
    const char* _str = str.c_str();
    if( !fs.isOpened() || !_str )
        return fs;
    if( *_str == '}' || *_str == ']' )
    {
        if( fs.structs.empty() )
            CV_Error_(Error::StsError,("Extra closing '%c'", *_str) );
        if( (*_str == ']' ? '[' : '{') != fs.structs.back() )
            CV_Error_( CV_StsError,
            ("The closing '%c' does not match the opening '%c'", *_str, fs.structs.back()));
        fs.structs.pop_back();
        fs.state = fs.structs.empty() || fs.structs.back() == '{' ?
            INSIDE_MAP + NAME_EXPECTED : VALUE_EXPECTED;
        cvEndWriteStruct( *fs );
        fs.elname = String();
    }
    else if( fs.state == NAME_EXPECTED + INSIDE_MAP )
    {
        if (!cv_isalpha(*_str) && *_str != '_')
            CV_Error_( CV_StsError, ("Incorrect element name %s", _str) );
        fs.elname = str;
        fs.state = VALUE_EXPECTED + INSIDE_MAP;
    }
    else if( (fs.state & 3) == VALUE_EXPECTED )
    {
        if( *_str == '{' || *_str == '[' )
        {
            fs.structs.push_back(*_str);
            int flags = *_str++ == '{' ? CV_NODE_MAP : CV_NODE_SEQ;
            fs.state = flags == CV_NODE_MAP ? INSIDE_MAP +
                NAME_EXPECTED : VALUE_EXPECTED;
            if( *_str == ':' )
            {
                flags |= CV_NODE_FLOW;
                _str++;
            }
            cvStartWriteStruct( *fs, fs.elname.size() > 0 ? fs.elname.c_str() : 0,
                flags, *_str ? _str : 0 );
            fs.elname = String();
        }
        else
        {
            write( fs, fs.elname, (_str[0] == '\\' && (_str[1] == '{' || _str[1] == '}' ||
                _str[1] == '[' || _str[1] == ']')) ? String(_str+1) : str );
            if( fs.state == INSIDE_MAP + VALUE_EXPECTED )
                fs.state = INSIDE_MAP + NAME_EXPECTED;
        }
    }
    else
        CV_Error( CV_StsError, "Invalid fs.state" );
    return fs;
}

FileNode FileStorage::operator[](const String& nodename) const
{
    return FileNode(fs, cvGetFileNodeByName(fs, 0, nodename.c_str()));
}


FileNode FileNode::operator[](const char* nodename) const
{
    return FileNode(fs, cvGetFileNodeByName(fs, node, nodename));
}

static const FileNodeIterator::SeqReader emptyReader = {0, 0, 0, 0, 0, 0, 0, 0};

FileNodeIterator::FileNodeIterator()
{
    fs = 0;
    container = 0;
    reader = emptyReader;
    remaining = 0;
}

FileNodeIterator::FileNodeIterator(const CvFileStorage* _fs,
                                   const CvFileNode* _node, size_t _ofs)
{
    reader = emptyReader;
    if( _fs && _node && CV_NODE_TYPE(_node->tag) != CV_NODE_NONE )
    {
        int node_type = _node->tag & FileNode::TYPE_MASK;
        fs = _fs;
        container = _node;
        if( !(_node->tag & FileNode::USER) && (node_type == FileNode::SEQ || node_type == FileNode::MAP) )
        {
            cvStartReadSeq( _node->data.seq, (CvSeqReader*)&reader );
            remaining = FileNode(_fs, _node).size();
        }
        else
        {
            reader.ptr = (schar*)_node;
            reader.seq = 0;
            remaining = 1;
        }
        (*this) += (int)_ofs;
    }
    else
    {
        fs = 0;
        container = 0;
        remaining = 0;
    }
}

FileNodeIterator::FileNodeIterator(const FileNodeIterator& it)
{
    fs = it.fs;
    container = it.container;
    reader = it.reader;
    remaining = it.remaining;
}

FileNodeIterator& FileNodeIterator::operator ++()
{
    if( remaining > 0 )
    {
        if( reader.seq )
        {
            if( ((reader).ptr += (((CvSeq*)reader.seq)->elem_size)) >= (reader).block_max )
            {
                cvChangeSeqBlock( (CvSeqReader*)&(reader), 1 );
            }
        }
        remaining--;
    }
    return *this;
}

FileNodeIterator FileNodeIterator::operator ++(int)
{
    FileNodeIterator it = *this;
    ++(*this);
    return it;
}

FileNodeIterator& FileNodeIterator::operator --()
{
    if( remaining < FileNode(fs, container).size() )
    {
        if( reader.seq )
        {
            if( ((reader).ptr -= (((CvSeq*)reader.seq)->elem_size)) < (reader).block_min )
            {
                cvChangeSeqBlock( (CvSeqReader*)&(reader), -1 );
            }
        }
        remaining++;
    }
    return *this;
}

FileNodeIterator FileNodeIterator::operator --(int)
{
    FileNodeIterator it = *this;
    --(*this);
    return it;
}

FileNodeIterator& FileNodeIterator::operator += (int ofs)
{
    if( ofs == 0 )
        return *this;
    if( ofs > 0 )
        ofs = std::min(ofs, (int)remaining);
    else
    {
        size_t count = FileNode(fs, container).size();
        ofs = (int)(remaining - std::min(remaining - ofs, count));
    }
    remaining -= ofs;
    if( reader.seq )
        cvSetSeqReaderPos( (CvSeqReader*)&reader, ofs, 1 );
    return *this;
}


void write( FileStorage& fs, const String& name, int value )
{ cvWriteInt( *fs, name.size() ? name.c_str() : 0, value ); }

void write( FileStorage& fs, const String& name, float value )
{ cvWriteReal( *fs, name.size() ? name.c_str() : 0, value ); }

void write( FileStorage& fs, const String& name, double value )
{ cvWriteReal( *fs, name.size() ? name.c_str() : 0, value ); }

void write( FileStorage& fs, const String& name, const String& value )
{ cvWriteString( *fs, name.size() ? name.c_str() : 0, value.c_str() ); }


#ifdef CV__LEGACY_PERSISTENCE

void read(const FileNode& node, std::vector<DMatch>& matches)
{
    FileNode first_node = *(node.begin());
    if (first_node.isSeq())
    {
        // modern scheme
#ifdef OPENCV_TRAITS_ENABLE_DEPRECATED
        FileNodeIterator it = node.begin();
        size_t total = (size_t)it.remaining;
        matches.resize(total);
        for (size_t i = 0; i < total; ++i, ++it)
        {
            (*it) >> matches[i];
        }
#else
        FileNodeIterator it = node.begin();
        it >> matches;
#endif
        return;
    }
    matches.clear();
    FileNodeIterator it = node.begin(), it_end = node.end();
    for( ; it != it_end; )
    {
        DMatch m;
        it >> m.queryIdx >> m.trainIdx >> m.imgIdx >> m.distance;
        matches.push_back(m);
    }
}
#endif

int FileNode::type() const { return !node ? NONE : (node->tag & TYPE_MASK); }

size_t FileNode::size() const
{
    int t = type();
    return t == MAP ? (size_t)((CvSet*)node->data.map)->active_count :
        t == SEQ ? (size_t)node->data.seq->total : (size_t)!isNone();
}

void read(const FileNode& node, int& value, int default_value)
{
    value = !node.node ? default_value :
    CV_NODE_IS_INT(node.node->tag) ? node.node->data.i : std::numeric_limits<int>::max();
}

void read(const FileNode& node, float& value, float default_value)
{
    value = !node.node ? default_value :
        CV_NODE_IS_INT(node.node->tag) ? (float)node.node->data.i :
        CV_NODE_IS_REAL(node.node->tag) ? saturate_cast<float>(node.node->data.f) : std::numeric_limits<float>::max();
}

void read(const FileNode& node, double& value, double default_value)
{
    value = !node.node ? default_value :
        CV_NODE_IS_INT(node.node->tag) ? (double)node.node->data.i :
        CV_NODE_IS_REAL(node.node->tag) ? node.node->data.f : std::numeric_limits<double>::max();
}

void read(const FileNode& node, String& value, const String& default_value)
{
    value = !node.node ? default_value : CV_NODE_IS_STRING(node.node->tag) ? String(node.node->data.str.ptr) : String();
}

} // namespace


/****************************************************************************
 * to_binary && binary_to
 ***************************************************************************/

template<typename _uint_t> inline size_t base64::
to_binary(_uint_t val, uchar * cur)
{
    size_t delta = CHAR_BIT;
    size_t cnt = sizeof(_uint_t);
    while (cnt --> static_cast<size_t>(0U)) {
        *cur++ = static_cast<uchar>(val);
        val >>= delta;
    }
    return sizeof(_uint_t);
}

template<> inline size_t base64::to_binary(double val, uchar * cur)
{
    Cv64suf bit64;
    bit64.f = val;
    return to_binary(bit64.u, cur);
}

template<> inline size_t base64::to_binary(float val, uchar * cur)
{
    Cv32suf bit32;
    bit32.f = val;
    return to_binary(bit32.u, cur);
}

template<typename _primitive_t> inline size_t base64::
to_binary(uchar const * val, uchar * cur)
{
    return to_binary<_primitive_t>(*reinterpret_cast<_primitive_t const *>(val), cur);
}


template<typename _uint_t> inline size_t base64::
binary_to(uchar const * cur, _uint_t & val)
{
    val = static_cast<_uint_t>(0);
    for (size_t i = static_cast<size_t>(0U); i < sizeof(_uint_t); i++)
        val |= (static_cast<_uint_t>(*cur++) << (i * CHAR_BIT));
    return sizeof(_uint_t);
}

template<> inline size_t base64::binary_to(uchar const * cur, double & val)
{
    Cv64suf bit64;
    binary_to(cur, bit64.u);
    val = bit64.f;
    return sizeof(val);
}

template<> inline size_t base64::binary_to(uchar const * cur, float & val)
{
    Cv32suf bit32;
    binary_to(cur, bit32.u);
    val = bit32.f;
    return sizeof(val);
}

template<typename _primitive_t> inline size_t base64::
binary_to(uchar const * cur, uchar * val)
{
    return binary_to<_primitive_t>(cur, *reinterpret_cast<_primitive_t *>(val));
}

class base64::BinaryToCvSeqConvertor
{
public:
    BinaryToCvSeqConvertor(const void* src, int len, const char* dt)
        : cur(reinterpret_cast<const uchar *>(src))
        , beg(reinterpret_cast<const uchar *>(src))
        , end(reinterpret_cast<const uchar *>(src))
    {
        CV_Assert(src);
        CV_Assert(dt);
        CV_Assert(len >= 0);

        /* calc binary_to_funcs */
        make_funcs(dt);
        functor_iter = binary_to_funcs.begin();

        step = ::icvCalcStructSize(dt, 0);
        end = beg + step * static_cast<size_t>(len);
    }

    inline BinaryToCvSeqConvertor & operator >> (CvFileNode & dst)
    {
        CV_DbgAssert(*this);

        /* get current data */
        union
        {
            uchar mem[sizeof(double)];
            uchar  u;
            char   b;
            ushort w;
            short  s;
            int    i;
            float  f;
            double d;
        } buffer; /* for GCC -Wstrict-aliasing */
        std::memset(buffer.mem, 0, sizeof(buffer));
        functor_iter->func(cur + functor_iter->offset, buffer.mem);

        /* set node::data */
        switch (functor_iter->cv_type)
        {
        case CV_8U : { dst.data.i = cv::saturate_cast<int>   (buffer.u); break;}
        case CV_8S : { dst.data.i = cv::saturate_cast<int>   (buffer.b); break;}
        case CV_16U: { dst.data.i = cv::saturate_cast<int>   (buffer.w); break;}
        case CV_16S: { dst.data.i = cv::saturate_cast<int>   (buffer.s); break;}
        case CV_32S: { dst.data.i = cv::saturate_cast<int>   (buffer.i); break;}
        case CV_32F: { dst.data.f = cv::saturate_cast<double>(buffer.f); break;}
        case CV_64F: { dst.data.f = cv::saturate_cast<double>(buffer.d); break;}
        default: break;
        }

        /* set node::tag */
        switch (functor_iter->cv_type)
        {
        case CV_8U :
        case CV_8S :
        case CV_16U:
        case CV_16S:
        case CV_32S: { dst.tag = CV_NODE_INT; /*std::printf("%i,", dst.data.i);*/ break; }
        case CV_32F:
        case CV_64F: { dst.tag = CV_NODE_REAL; /*std::printf("%.1f,", dst.data.f);*/ break; }
        default: break;
        }

        /* check if end */
        if (++functor_iter == binary_to_funcs.end()) {
            functor_iter = binary_to_funcs.begin();
            cur += step;
        }

        return *this;
    }

    inline operator bool() const
    {
        return cur < end;
    }

private:
    typedef size_t(*binary_to_t)(uchar const *, uchar *);
    struct binary_to_filenode_t
    {
        size_t      cv_type;
        size_t      offset;
        binary_to_t func;
    };

private:
    void make_funcs(const char* dt)
    {
        size_t cnt = 0;
        char type = '\0';
        size_t offset = 0;

        std::istringstream iss(dt);
        while (!iss.eof()) {
            if (!(iss >> cnt)) {
                iss.clear();
                cnt = 1;
            }
            CV_Assert(cnt > 0U);
            if (!(iss >> type))
                break;

            while (cnt-- > 0)
            {
                binary_to_filenode_t pack;

                /* set func and offset */
                size_t size = 0;
                switch (type)
                {
                case 'u':
                case 'c':
                    size      = sizeof(uchar);
                    pack.func = binary_to<uchar>;
                    break;
                case 'w':
                case 's':
                    size      = sizeof(ushort);
                    pack.func = binary_to<ushort>;
                    break;
                case 'i':
                    size      = sizeof(uint);
                    pack.func = binary_to<uint>;
                    break;
                case 'f':
                    size      = sizeof(float);
                    pack.func = binary_to<float>;
                    break;
                case 'd':
                    size      = sizeof(double);
                    pack.func = binary_to<double>;
                    break;
                case 'r':
                default:  { CV_Assert(!"type not support"); break; }
                }; // need a better way for outputting error.

                offset = static_cast<size_t>(cvAlign(static_cast<int>(offset), static_cast<int>(size)));
                pack.offset = offset;
                offset += size;

                /* set type */
                switch (type)
                {
                case 'u': { pack.cv_type = CV_8U ; break; }
                case 'c': { pack.cv_type = CV_8S ; break; }
                case 'w': { pack.cv_type = CV_16U; break; }
                case 's': { pack.cv_type = CV_16S; break; }
                case 'i': { pack.cv_type = CV_32S; break; }
                case 'f': { pack.cv_type = CV_32F; break; }
                case 'd': { pack.cv_type = CV_64F; break; }
                case 'r':
                default:  { CV_Assert(!"type is not support"); break; }
                } // need a better way for outputting error.

                binary_to_funcs.push_back(pack);
            }
        }

        CV_Assert(iss.eof());
        CV_Assert(binary_to_funcs.size());
    }

private:

    const uchar * cur;
    const uchar * beg;
    const uchar * end;

    size_t step;
    std::vector<binary_to_filenode_t> binary_to_funcs;
    std::vector<binary_to_filenode_t>::iterator functor_iter;
};


/****************************************************************************
 * constant
 ***************************************************************************/
#if CHAR_BIT != 8
#error "`char` should be 8 bit."
#endif

base64::uint8_t const base64::base64_mapping[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

base64::uint8_t const base64::base64_padding = '=';

base64::uint8_t const base64::base64_demapping[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0, 62,  0,  0,  0, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,  0,  0,
    0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0,  0,  0,  0,  0,  0, 26, 27, 28,
    29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
    49, 50, 51,  0,  0,  0,  0,
};


/*    `base64_demapping` above is generated in this way:
 *    `````````````````````````````````````````````````````````````````````
 *  std::string mapping((const char *)base64_mapping);
 *    for (auto ch = 0; ch < 127; ch++) {
 *        auto i = mapping.find(ch);
 *        printf("%3u, ", (i != std::string::npos ? i : 0));
 *    }
 *    putchar('\n');
 *    `````````````````````````````````````````````````````````````````````
 */



bool base64::base64_valid(uint8_t const * src, size_t off, size_t cnt)
{
    /* check parameters */
    if (src == 0 || src + off == 0)
        return false;
    if (cnt == 0U)
        cnt = std::strlen(reinterpret_cast<char const *>(src));
    if (cnt == 0U)
        return false;
    if (cnt & 0x3U)
        return false;

    /* initialize beginning and end */
    uint8_t const * beg = src + off;
    uint8_t const * end = beg + cnt;

    /* skip padding */
    if (*(end - 1U) == base64_padding) {
        end--;
        if (*(end - 1U) == base64_padding)
            end--;
    }

    /* find illegal characters */
    for (uint8_t const * iter = beg; iter < end; iter++)
        if (*iter > 126U || (!base64_demapping[(uint8_t)*iter] && *iter != base64_mapping[0]))
            return false;

    return true;
}

bool base64::base64_valid(char const * src, size_t off, size_t cnt)
{
    if (cnt == 0U)
        cnt = std::strlen(src);

    return base64_valid(reinterpret_cast<uint8_t const *>(src), off, cnt);
}


size_t base64::base64_decode(uint8_t const * src, uint8_t * dst, size_t off, size_t cnt)
{
    /* check parameters */
    if (!src || !dst || !cnt)
        return 0U;
    if (cnt & 0x3U)
        return 0U;

    /* initialize beginning and end */
    uint8_t       * dst_beg = dst;
    uint8_t       * dst_cur = dst_beg;

    uint8_t const * src_beg = src + off;
    uint8_t const * src_cur = src_beg;
    uint8_t const * src_end = src_cur + cnt;

    /* start decoding */
    while (src_cur < src_end) {
        uint8_t d50 = base64_demapping[*src_cur++];
        uint8_t c50 = base64_demapping[*src_cur++];
        uint8_t b50 = base64_demapping[*src_cur++];
        uint8_t a50 = base64_demapping[*src_cur++];

        uint8_t b10 = b50 & 0x03U;
        uint8_t b52 = b50 & 0x3CU;
        uint8_t c30 = c50 & 0x0FU;
        uint8_t c54 = c50 & 0x30U;

        *dst_cur++ = (d50 << 2U) | (c54 >> 4U);
        *dst_cur++ = (c30 << 4U) | (b52 >> 2U);
        *dst_cur++ = (b10 << 6U) | (a50 >> 0U);
    }

    *dst_cur = 0;
    return size_t(dst_cur - dst_beg);
}

size_t base64::base64_decode(char const * src, char * dst, size_t off, size_t cnt)
{
    if (cnt == 0U)
        cnt = std::strlen(src);

    return base64_decode
    (
        reinterpret_cast<uint8_t const *>(src),
        reinterpret_cast<uint8_t       *>(dst),
        off,
        cnt
    );
}

size_t base64::base64_decode_buffer_size(size_t cnt, bool is_end_with_zero)
{
    size_t additional = static_cast<size_t>(is_end_with_zero == true);
    return cnt / 4U * 3U + additional;
}

size_t base64::base64_decode_buffer_size(size_t cnt, char  const * src, bool is_end_with_zero)
{
    return base64_decode_buffer_size(cnt, reinterpret_cast<uchar const *>(src), is_end_with_zero);
}

size_t base64::base64_decode_buffer_size(size_t cnt, uchar const * src, bool is_end_with_zero)
{
    size_t padding_cnt = 0U;
    for (uchar const * ptr = src + cnt - 1U; *ptr == base64_padding; ptr--)
        padding_cnt ++;
    return base64_decode_buffer_size(cnt, is_end_with_zero) - padding_cnt;
}

size_t base64::base64_encode_buffer_size(size_t cnt, bool is_end_with_zero)
{
    size_t additional = static_cast<size_t>(is_end_with_zero == true);
    return (cnt + 2U) / 3U * 4U + additional;
}

bool base64::read_base64_header(std::vector<char> const & header, std::string & dt)
{
    std::istringstream iss(header.data());
    return !!(iss >> dt);//the "std::basic_ios::operator bool" differs between C++98 and C++11. The "double not" syntax is portable and covers both cases with equivalent meaning
}

/****************************************************************************
 * Emitter
 ***************************************************************************/

/* A decorator for CvFileStorage
 * - no copyable
 * - not safe for now
 * - move constructor may be needed if C++11
 */
class base64::Base64ContextEmitter
{
public:
    explicit Base64ContextEmitter(CvFileStorage * fs)
        : file_storage(fs)
        , binary_buffer(BUFFER_LEN)
        , base64_buffer(base64_encode_buffer_size(BUFFER_LEN))
        , src_beg(0)
        , src_cur(0)
        , src_end(0)
    {
        src_beg = binary_buffer.data();
        src_end = src_beg + BUFFER_LEN;
        src_cur = src_beg;

        CV_CHECK_OUTPUT_FILE_STORAGE(fs);

        if ( fs->fmt == CV_STORAGE_FORMAT_JSON )
        {
            /* clean and break buffer */
            *fs->buffer++ = '\0';
            ::icvPuts( fs, fs->buffer_start );
            fs->buffer = fs->buffer_start;
            memset( file_storage->buffer_start, 0, static_cast<int>(file_storage->space) );
            ::icvPuts( fs, "\"$base64$" );
        }
        else
        {
            ::icvFSFlush(file_storage);
        }
    }
private:
    /* because of Base64, we must keep its length a multiple of 3 */
    static const size_t BUFFER_LEN = 48U;
    // static_assert(BUFFER_LEN % 3 == 0, "BUFFER_LEN is invalid");

private:
    CvFileStorage * file_storage;

    std::vector<uchar> binary_buffer;
    std::vector<uchar> base64_buffer;
    uchar * src_beg;
    uchar * src_cur;
    uchar * src_end;
};


/****************************************************************************
 * Wapper
 ***************************************************************************/

void base64::make_seq(void * binary, int elem_cnt, const char * dt, ::CvSeq & seq)
{
    ::CvFileNode node;
    node.info = 0;
    BinaryToCvSeqConvertor convertor(binary, elem_cnt, dt);
    while (convertor) {
        convertor >> node;
        cvSeqPush(&seq, &node);
    }
}


base64::Base64Writer::Base64Writer(::CvFileStorage * fs)
    : emitter(new Base64ContextEmitter(fs))
    , data_type_string()
{
    CV_CHECK_OUTPUT_FILE_STORAGE(fs);
}

base64::Base64Writer::~Base64Writer()
{
    delete emitter;
}

/****************************************************************************
 * Parser
 ***************************************************************************/

base64::Base64ContextParser::Base64ContextParser(uchar * buffer, size_t size)
    : dst_cur(buffer)
    , dst_end(buffer + size)
    , base64_buffer(BUFFER_LEN)
    , src_beg(0)
    , src_cur(0)
    , src_end(0)
    , binary_buffer(base64_encode_buffer_size(BUFFER_LEN))
{
    src_beg = binary_buffer.data();
    src_cur = src_beg;
    src_end = src_beg + BUFFER_LEN;
}

base64::Base64ContextParser::~Base64ContextParser()
{
    /* encode the rest binary data to base64 buffer */
    if (src_cur != src_beg)
        flush();
}

base64::Base64ContextParser & base64::Base64ContextParser::
read(const uchar * beg, const uchar * end)
{
    if (beg >= end)
        return *this;

    while (beg < end) {
        /* collect binary data and copy to binary buffer */
        size_t len = std::min(end - beg, src_end - src_cur);
        std::memcpy(src_cur, beg, len);
        beg     += len;
        src_cur += len;

        if (src_cur >= src_end) {
            /* binary buffer is full. */
            /* decode it send result to dst */

            CV_Assert(flush());    /* check for base64_valid */
        }
    }

    return *this;
}

bool base64::Base64ContextParser::flush()
{
    if ( !base64_valid(src_beg, 0U, src_cur - src_beg) )
        return false;

    if ( src_cur == src_beg )
        return true;

    uchar * buffer = binary_buffer.data();
    size_t len = base64_decode(src_beg, buffer, 0U, src_cur - src_beg);
    src_cur = src_beg;

    /* unexpected error */
    CV_Assert(len != 0);

    /* buffer is full */
    CV_Assert(dst_cur + len < dst_end);

    if (dst_cur + len < dst_end) {
        /* send data to dst */
        std::memcpy(dst_cur, buffer, len);
        dst_cur += len;
    }

    return true;
}

CV_IMPL CvString
cvMemStorageAllocString( CvMemStorage* storage, const char* ptr, int len )
{
    CvString str;
    memset(&str, 0, sizeof(CvString));

    str.len = len >= 0 ? len : (int)strlen(ptr);
    str.ptr = (char*)cvMemStorageAlloc( storage, str.len + 1 );
    memcpy( str.ptr, ptr, str.len );
    str.ptr[str.len] = '\0';

    return str;
}

namespace cv
{
void DefaultDeleter<CvFileStorage>::operator ()(CvFileStorage* obj) const { cvReleaseFileStorage(&obj); }

}
