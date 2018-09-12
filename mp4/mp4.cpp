#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <vector>

#define RR(x, n)                (((x) >> (n)) | ((x) << (32 - n)))
#define SWAP_BIT_U8(u8)         (u8)
#define SWAP_BIT_U16(u16)       \
    (((((uint16_t)(u16)) >> 8) & 0xFF) | \
    ((((uint16_t)(u16)) & 0xFF) << 8))

#define SWAP_BIT_U32(u32)                    \
    (RR((uint32_t)(u32), 24) & 0x00FF00FF) | \
    (RR((uint32_t)(u32), 8) & 0xFF00FF00)

#define SWAP_BIT_U64(u64)       \
    ((((uint64_t)(SWAP_BIT_U32((u64) & 0xFFFFFFFF)) << 32) & 0xFFFFFFFF00000000) \
        | SWAP_BIT_U32(((u64) >> 32) & 0xFFFFFFFF))

void add_tab(int ntab)
{
    int off = 0;
    char tab[100];
    for (int i = 0; i < ntab; i++) {
        off += sprintf(tab + off, "\t");
    }
    printf("%s", tab);
}

void print(int deep, const char * format, ...)
{
    if (deep > 0) {
        add_tab(deep);
    }
    
    char line[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(line, 1024, format, args);
    va_end(args);
    
    printf("%s", line);
}

bool is_little_endian(void)
{
    static bool check = false;
    static bool little_endian = true;
    
    if (check) {
        return little_endian;
    }
    
    union {
        uint32_t    u32;
        struct {
            uint8_t a;
            uint8_t b;
            uint8_t c;
            uint8_t d;
        } s32;
    } flag;
    
    flag.u32 = 0x00000001;
    check = true;
    little_endian = (flag.s32.a == 0x01);
    return little_endian;
}

// mp4 data, default big endian
template <typename T>
union nbytes_t {
    
    uint8_t sz_val[sizeof(T) + 1];
    T i_val;
    
//    nbytes_t(void) {
//        i_val = 0;
//    }
    
    void clear(void) {
        memset(sz_val, 0, sizeof(T) + 1);
    }
    
    T little_endian_val(void) const {
        if (is_little_endian()) {
#if 0
            if (sizeof(T) == 4) {
                uint32_t value = 0;
                value = (value << 8) | sz_val[0];
                value = (value << 8) | sz_val[1];
                value = (value << 8) | sz_val[2];
                value = (value << 8) | sz_val[3];
                return value;
            } else if (sizeof(T) == 2) {
                uint16_t value = 0;
                value = (value << 8) | sz_val[0];
                value = (value << 8) | sz_val[1];
                return value;
            } else if (sizeof(T) == 8) {
                uint64_t value = 0;
                value = (value << 8) | sz_val[7];
                value = (value << 8) | sz_val[6];
                value = (value << 8) | sz_val[5];
                value = (value << 8) | sz_val[4];
                value = (value << 8) | sz_val[3];
                value = (value << 8) | sz_val[2];
                value = (value << 8) | sz_val[1];
                value = (value << 8) | sz_val[0];
                return value;
            }
#else
            if (sizeof(T) == 4) {
                return SWAP_BIT_U32(i_val);
            } else if (sizeof(T) == 2) {
                return SWAP_BIT_U16(i_val);
            } else if (sizeof(T) == 8) {
                return SWAP_BIT_U64(i_val);
            }
#endif
        }
        return i_val;
    }
    
    int big_endian_val(void) const {
        if (!is_little_endian()) {
            if (sizeof(T) == 4) {
                return SWAP_BIT_U32(i_val);
            } else if (sizeof(T) == 2) {
                return SWAP_BIT_U16(i_val);
            } else if (sizeof(T) == 8) {
                return SWAP_BIT_U64(i_val);
            }
        }
        return i_val;
    }
    
    const uint8_t * value(void) const {
        return sz_val;
    }
    
    int read(int fd) {
        clear();
        return ::read(fd, sz_val, sizeof(T));
    }
    
    int nbytes(void) const {
        return sizeof(T);
    }
};

struct boxhead_t {
private:
    union {
        nbytes_t<long> l_val;
        nbytes_t<int> i_val;
    } _size;
    nbytes_t<int> _type;;
    bool _ok;
    bool _large_size;

public:
    boxhead_t(void) {
        clear();
    }
    
    void clear(void) {
        _size.l_val.clear();
        _ok = false;
        _large_size = false;
        _type.clear();
    }
    
    int read(int fd) {
        _ok = false;
        _large_size = false;
        
        int nread = _size.i_val.read(fd);        
        if (nread != _size.i_val.nbytes()) {
            return nread;
        }
        nread = _type.read(fd);
        if (nread != _type.nbytes()) {
            return nread;
        }
        if (_size.i_val.little_endian_val() == 1) { //read large size
            _large_size = true;
            nread = _size.l_val.read(fd);  
            if (nread != _size.l_val.nbytes()) {
                return nread;
            }
        }
        _ok = true;
        return head_bytes();
    }
    
    long size(void) const {
        if (_large_size) {
            return _size.l_val.little_endian_val();
        } else {
            return _size.i_val.little_endian_val();
        }
    }
    
    int head_bytes(void) const {
        return _size.i_val.nbytes() + _type.nbytes() + (_large_size ? 8 : 0);
    }
    
    const char * type(void) const {
        return (const char *)_type.value();
    }
    
    bool is_ok(void) const {
        return _ok;
    }
};

struct ftyp_box_t {
    nbytes_t<int> major_brand;
    nbytes_t<int> minor_version;
    int n_compatible_brand;
    nbytes_t<int> compatible_brand[10];
    
    void show(int level) const {
        print(level, "major_brand:%s\n", major_brand.value());
        print(level, "minor_version:%d\n", minor_version.little_endian_val());
        
        char buf[1024];
        int off = 0;
        for (int i = 0 ; i < n_compatible_brand; i++) {
            off += sprintf(buf + off, "%s, ", compatible_brand[i].value());
        }
        if (n_compatible_brand > 0) {
            buf[off - 2] = '\0';
        }
        print(level, "compatible_brand:%s\n", buf);
    }
    
    int read(int fd, int size) {
        int nread = major_brand.read(fd);
        if (nread != major_brand.nbytes()) {
            return -1;
        }
        
        nread = minor_version.read(fd);
        if (nread != minor_version.nbytes()) {
            return -1;
        }
        
        int nremain = size - major_brand.nbytes() - minor_version.nbytes();
        n_compatible_brand = nremain / 4;
        for (int i = 0; i < n_compatible_brand; i++) {
            nread = compatible_brand[i].read(fd);
            if (nread != compatible_brand[i].nbytes()) {
                return -1;
            }
        }
        
        return size;
    }
};


struct mvhd_box_t {
    nbytes_t<int> version_flag;
    union {
        nbytes_t<int> i;
        nbytes_t<long> l;
    } creation_time;
    union {
        nbytes_t<int> i;
        nbytes_t<long> l;
    } modification_time;
    nbytes_t<int> timescale;
    union {
        nbytes_t<int> i;
        nbytes_t<long> l;
    } duration;
    nbytes_t<int> rate;
    nbytes_t<short> volume;
    nbytes_t<short> bit;
    nbytes_t<int> reserved[2];
    nbytes_t<int> matrix[9];
    nbytes_t<int> pre_defined[6];
    nbytes_t<int> next_track_ID;
    
    int read(int fd, int size) {
        int nread = version_flag.read(fd);
        if (nread != version_flag.nbytes()) {
            return -1;
        }
        int version = version_flag.value()[0];
        if (version == 1) {
            nread = creation_time.l.read(fd);
            nread = modification_time.l.read(fd);
            nread = timescale.read(fd);
            nread = duration.l.read(fd);
        } else {
            nread = creation_time.i.read(fd);
            nread = modification_time.i.read(fd);
            nread = timescale.read(fd);
            nread = duration.i.read(fd);
        }
        nread = rate.read(fd);
        nread = volume.read(fd);
        nread = bit.read(fd);
        for (int i = 0; i < 2; i++) {
            nread = reserved[i].read(fd);            
        }
        for (int i = 0; i < 9; i++) {
            nread = matrix[i].read(fd);            
        }
        for (int i = 0; i < 6; i++) {
            nread = pre_defined[i].read(fd);            
        }
        nread = next_track_ID.read(fd);
        
        return size;
    }
    
    int version(void) const {
        return version_flag.value()[0];
    }
    
    void show(int level) {
        print(level, "version:%u\n", version());
        double file_duration = 0.0;
        if (version() == 1) {
            print(level, "creation_time:%ld\n", creation_time.l.little_endian_val());
            print(level, "modification_time:%ld\n", modification_time.l.little_endian_val());
            print(level, "timescale:%d\n", timescale.little_endian_val());
            print(level, "duration:%ld\n", duration.l.little_endian_val());
            file_duration = (double)duration.l.little_endian_val() / (double)timescale.little_endian_val();
        } else {
            print(level, "creation_time:%d\n", creation_time.i.little_endian_val());
            print(level, "modification_time:%d\n", modification_time.i.little_endian_val());
            print(level, "timescale:%d\n", timescale.little_endian_val());
            print(level, "duration:%d\n", duration.i.little_endian_val());
            file_duration = (double)duration.i.little_endian_val() / (double)timescale.little_endian_val();
        }
        print(level, "rate:%d\n", rate.little_endian_val());
        print(level, "volume:%d\n", volume.big_endian_val());    ///???
        print(level, "bit:%d\n", bit.little_endian_val());
        print(level, "next_track_ID:%u\n", next_track_ID.little_endian_val());
        print(level, "file duration:%f\n", file_duration);
    }
};

struct tkhd_box_t
{
    nbytes_t<int> version_flag;
    union {
        nbytes_t<int> i;
        nbytes_t<long> l;
    } creation_time;
    union {
        nbytes_t<int> i;
        nbytes_t<long> l;
    } modification_time;
    nbytes_t<int> track_ID;
    nbytes_t<int> reserved;
    union {
        nbytes_t<int> i;
        nbytes_t<long> l;
    } duration;
    nbytes_t<int> reserved1[2];
    nbytes_t<short> layer;
    nbytes_t<short> alternate_group;
    nbytes_t<short> volume;
    nbytes_t<short> reserved2;
    nbytes_t<int> matrix[9];
    nbytes_t<int> width;
    nbytes_t<int> height;
    
    int read(int fd, int size) {
        int nread = version_flag.read(fd);
        if (nread != version_flag.nbytes()) {
            return -1;
        }
        uint32_t v = version();
        uint32_t f = flag();
        
        if (v == 1) {
            nread = creation_time.l.read(fd);
            nread = modification_time.l.read(fd);
            nread = track_ID.read(fd);
            nread = reserved.read(fd);
            nread = duration.l.read(fd);
        } else {
            nread = creation_time.i.read(fd);
            nread = modification_time.i.read(fd);
            nread = track_ID.read(fd);
            nread = reserved.read(fd);
            nread = duration.i.read(fd);
        }
        for (int i = 0; i < 2; i++) {
            nread = reserved1[i].read(fd);            
        }
        nread = layer.read(fd);
        nread = alternate_group.read(fd);
        nread = volume.read(fd);
        nread = reserved2.read(fd);
        for (int i = 0; i < 9; i++) {
            nread = matrix[i].read(fd);            
        }
        nread = width.read(fd);
        nread = height.read(fd);
        
        return size;
    }
    
    int version(void) const {
        return version_flag.value()[0];
    }
    
    uint32_t flag(void) const {
        uint32_t f = 0;
        f = (f << 8) | version_flag.value()[1];
        f = (f << 8) | version_flag.value()[2];
        f = (f << 8) | version_flag.value()[3];
        
        //    Track_enabled: Indicates that the track is enabled.  
        //若此位为0，则该track内容无需播放（比如我们用一些非线编软件<如Sony Vegas>做视频剪辑时，有些Track仅为我们参考与模仿用，在输出时将该Track关掉）。
        //    Track_in_movie: Indicates that the track is used in the presentation. 
        //    Track_in_preview: Indicates that the track is used when previewing the presentation.
        //        
        //    Track_in_poster: Indicates that the track is used in movie's poster.
            
        return f;
    }
    
    bool need_play(void) const {
        uint32_t f = flag();
        return f == 1;
    }
    
    void show(int level) {
        print(level, "version:%u\n", version());
        double file_duration = 0.0;
        if (version() == 1) {
            print(level, "creation_time:%ld\n", creation_time.l.little_endian_val());
            print(level, "modification_time:%ld\n", modification_time.l.little_endian_val());
            print(level, "duration:%d\n", duration.l.little_endian_val());
        } else {
            print(level, "creation_time:%d\n", creation_time.i.little_endian_val());
            print(level, "modification_time:%d\n", modification_time.i.little_endian_val());
            print(level, "duration:%d\n", duration.i.little_endian_val());
        }
        
        print(level, "duration:%d\n", duration.i.little_endian_val());
        print(level, "track_id:%d\n", track_ID.little_endian_val());
        print(level, "layer:%d\n", layer.big_endian_val());
        print(level, "alternate_group:%d\n", alternate_group.big_endian_val());
        print(level, "volume:%d\n", volume.big_endian_val());
        print(level, "width:%d\n", width.little_endian_val());
        print(level, "height:%d\n", height.little_endian_val());
    }
};

struct edit_list_t
{
    nbytes_t<uint32_t> segment_duration;
    nbytes_t<uint32_t> media_time;
    nbytes_t<uint32_t> media_rate;
    
    int read(int fd) {
        int nread = segment_duration.read(fd);
        nread = media_time.read(fd);
        nread = media_rate.read(fd);
        return 12;
    }
};

struct elts_box_t
{
    nbytes_t<uint32_t> version_flag;
    nbytes_t<int> amount;
    edit_list_t edit_list[10];
    
    int read(int fd, int size) {
        int nread = version_flag.read(fd);
        if (nread != version_flag.nbytes()) {
            return -1;
        }
        nread = amount.read(fd);
        for (int i = 0; i < amount.little_endian_val(); i++) {
            nread = edit_list[i].read(fd);
        }
        return size;
    }
    
    void show(int level) {
        for (int i = 0; i < amount.little_endian_val(); i++) {
            print(level, "edit_list[%d]\n", i);
            print(level + 1, "segment_duration:%u\n", edit_list[i].segment_duration.little_endian_val());
            print(level + 1, "media_time:%u\n", edit_list[i].media_time.little_endian_val());
            print(level + 1, "media_rate:%u\n", edit_list[i].media_rate.little_endian_val());

        }
    }
};

//media header box
struct mdhd_box_t
{
    nbytes_t<uint32_t> version_flag;
    nbytes_t<uint32_t> creation_time;
    nbytes_t<uint32_t> modification_time;
    nbytes_t<uint32_t> timescale;
    nbytes_t<uint32_t> duration;
    nbytes_t<uint16_t> language;
    nbytes_t<uint16_t> pre_defined;
    
    int read(int fd, int size) {
        int nread;
        
        nread = version_flag.read(fd);
        nread = creation_time.read(fd);
        nread = modification_time.read(fd);
        nread = timescale.read(fd);
        nread = duration.read(fd);
        nread = language.read(fd);
        nread = pre_defined.read(fd);
        
        return size;
    }
    
    void show(int level) {
        print(level, "creation_time:%u\n", creation_time.little_endian_val());
        print(level, "modification_time:%u\n", modification_time.little_endian_val());
        print(level, "timescale:%u\n", timescale.little_endian_val());
        print(level, "duration:%u\n", duration.little_endian_val());
        print(level, "language:%u\n", language.little_endian_val());
        print(level, "pre_defined:%u\n", pre_defined.little_endian_val());
    }
};

struct hdlr_box_t {
    nbytes_t<uint32_t> version_flag;
    nbytes_t<uint32_t> pre_defined;
    nbytes_t<uint32_t> handler_type;
    nbytes_t<uint32_t> reserved[3];
    char name[1024];
    
    int read(int fd, int size) {
        int nread;
        
        nread = version_flag.read(fd);
        nread = pre_defined.read(fd);
        nread = handler_type.read(fd);
        for (int i = 0; i < 3; i++) {
            nread = reserved[i].read(fd);
        }
        int remain = size - version_flag.nbytes() - pre_defined.nbytes() 
            - handler_type.nbytes() - 3 * reserved[0].nbytes();
        nread = ::read(fd, name, remain);
        
        return size;
    }
    
    void show(int level) {
        print(level, "pre_defined:%u\n", pre_defined.little_endian_val());
        print(level, "handler_type:%s\n", handler_type.value());
        print(level, "name:%s\n", name);
    }
};

//Video Media Header Box
struct vmhd_box_t
{
    nbytes_t<uint32_t> version_flag;
    nbytes_t<uint32_t> graphics_mode;
    nbytes_t<uint32_t> opcolor;
    
    int read(int fd, int size) {
        int nread;
        
        nread = version_flag.read(fd);
        nread = graphics_mode.read(fd);
        nread = opcolor.read(fd);
        
        return size;
    }
    
    void show(int level) {
        print(level, "graphics_mode:%u\n", graphics_mode.little_endian_val());
        print(level, "opcolor:0x%08x\n", opcolor.little_endian_val());
    }
};

//Sound Media Header Box
struct smhd_box_t
{
    nbytes_t<uint32_t> version_flag;
    nbytes_t<uint16_t> balance;
    nbytes_t<uint16_t> reserved;
    
    int read(int fd, int size) {
        int nread;
        
        nread = version_flag.read(fd);
        nread = balance.read(fd);
        nread = reserved.read(fd);
        
        return size;
    }
    
    void show(int level) {
        print(level, "balance:%u\n", balance.little_endian_val());
    }
};

struct urls_box_t   //location string
{
    nbytes_t<uint32_t> version_flag;
    char url[1024];
    
    int read(int fd, int size) {
        int nread;
        
        nread = version_flag.read(fd);
        if (flag() != 1) {
            ::read(fd, url, size - version_flag.nbytes());
        }
        
        return size;
    }
    
    uint32_t flag(void) const {
        uint32_t f = 0;
        f = (f << 8) | version_flag.value()[1];
        f = (f << 8) | version_flag.value()[2];
        f = (f << 8) | version_flag.value()[3];
        return f;
    }
};

struct urns_box_t   //name string and location string
{
    nbytes_t<uint32_t> version_flag;
};

struct dref_box_t 
{
    nbytes_t<uint32_t> version_flag;
    nbytes_t<uint32_t> entry_count;
    union {
        urls_box_t url[10];
        urns_box_t urn[10];
    } entry;
    
    int read(int fd, int size) {
        int nread;
        
        nread = version_flag.read(fd);
        nread = entry_count.read(fd);
        
        boxhead_t head;
        for (int i = 0; i < entry_count.little_endian_val(); i++) {
            head.read(fd);
            if (strncmp(head.type(), "url ", 4) == 0) {
                entry.url[i].read(fd, head.size() - head.head_bytes());
            } else {
                //print(level + 1, "urn");
            }
        }
        
        return size;
    }
    
    void show(int level) {
        print(level, "entry_count:%d\n", entry_count.little_endian_val());
    }
};

struct SequenceParameterSet_t
{
    nbytes_t<uint16_t> sequenceParameterSetLength;
    uint8_t sequenceParameterSetNALUnit[1024];
    
    int read(int fd, int size) {
        sequenceParameterSetLength.read(fd);
        ::read(fd, sequenceParameterSetNALUnit, 
               sequenceParameterSetLength.little_endian_val());
        
        return size;
    }
};

struct PictureParameterSet_t
{
    nbytes_t<uint16_t> pictureParameterSetLength;
    uint8_t pictureParameterSetNALUnit[1024];
    
    int read(int fd, int size) {
        pictureParameterSetLength.read(fd);
        ::read(fd, pictureParameterSetNALUnit, 
               pictureParameterSetLength.little_endian_val());
        
        return size;
    }
};

struct avcc_box_t 
{
    nbytes_t<uint8_t> configurationVersion;
    nbytes_t<uint8_t> AVCProfileIndication;
    nbytes_t<uint8_t> profile_compatibility;
    nbytes_t<uint8_t> AVCLevelIndication;
    //nbytes_t<uint8_t> lengthSizeMinusOne;
    uint8_t lengthSizeMinusOne;
    uint8_t numOfSequenceParameterSets;
    //nbytes_t<uint8_t> numOfSequenceParameterSets;
    SequenceParameterSet_t sps[10];
    nbytes_t<uint8_t> numOfPictureParameterSets;
    PictureParameterSet_t pps[10];
    
    int read(int fd, int size) {
        configurationVersion.read(fd);
        AVCProfileIndication.read(fd);
        profile_compatibility.read(fd);
        AVCLevelIndication.read(fd);
        
        nbytes_t<uint8_t> tmp;
        tmp.read(fd);
        lengthSizeMinusOne = tmp.i_val & 0x03;
        
        tmp.read(fd);
        numOfSequenceParameterSets = tmp.i_val & 0x1F;
        for (int i = 0; i < numOfSequenceParameterSets; i++) {
            sps[i].read(fd, size);
        }
        
        numOfPictureParameterSets.read(fd);
        for (int i = 0; i < numOfPictureParameterSets.i_val; i++) {
            pps[i].read(fd, size);
        }
        
        return size;
    }
    
    void show(int level) {
        print(level, "configurationVersion:%u\n", configurationVersion.i_val);
        print(level, "AVCProfileIndication:%u\n", AVCProfileIndication.i_val);
        print(level, "profile_compatibility:%u\n", profile_compatibility.i_val);
        print(level, "AVCLevelIndication:%u\n", AVCLevelIndication.i_val);
        print(level, "lengthSizeMinusOne:%u\n", lengthSizeMinusOne);
        print(level, "numOfSequenceParameterSets:%u\n", numOfSequenceParameterSets);
        print(level, "numOfPictureParameterSets:%u\n", numOfPictureParameterSets);
    }
};

struct pasp_box_t 
{
    nbytes_t<uint32_t> hSpacing;
    nbytes_t<uint32_t> vSpacing;
    
    int read(int fd, int size) {
        int nread;
        
        nread = hSpacing.read(fd);
        nread = vSpacing.read(fd);
        
        return size;
    }
    
    void show(int level) {
        print(level, "hSpacing:%u\n", hSpacing.little_endian_val());
        print(level, "vSpacing:%u\n", vSpacing.little_endian_val());
    }
};

struct visual_sample_entry_t
{
    uint8_t reserved0[6];    //6bytes
    nbytes_t<uint16_t> data_reference_index;
    nbytes_t<uint16_t> pre_defined1;
    nbytes_t<uint16_t> reserved;
    uint32_t pre_defined2[3 * 4];
    //nbytes_t<uint32_t> pre_defined2[3];
    nbytes_t<uint16_t> width;
    nbytes_t<uint16_t> height;
    nbytes_t<uint32_t> horizresolution;
    nbytes_t<uint32_t> vertresolution;
    nbytes_t<uint32_t> reserved1;
    nbytes_t<uint16_t> frame_count;
    uint8_t compressorname[33];
    nbytes_t<uint16_t> depth;
    nbytes_t<uint16_t> pre_defined3;
    
    avcc_box_t avcc_box;
    pasp_box_t pasp_box;
    bool has_avcc;
    bool has_pasp;
    
    int read(int fd, int size) {
        has_avcc = false;
        has_pasp = false;
        int nread = 0;
        nread += ::read(fd, reserved0, 6);  
        nread += data_reference_index.read(fd);
        nread += pre_defined1.read(fd);
        nread += reserved.read(fd);
        nread += ::read(fd, pre_defined2, 12);
        nread += width.read(fd);
        nread += height.read(fd);
        nread += horizresolution.read(fd);
        nread += vertresolution.read(fd);
        nread += reserved1.read(fd);
        nread += frame_count.read(fd);
        nread += ::read(fd, compressorname, 32);
        nread += depth.read(fd);
        nread += pre_defined3.read(fd);
        
        while (size - nread > 0) {
            boxhead_t head;
            head.read(fd);
            if (strncmp(head.type(), "avcC", 4) == 0) {
                avcc_box.read(fd, head.size());
                has_avcc = true;
            } else if (strncmp(head.type(), "pasp", 4) == 0) {
                pasp_box.read(fd, head.size());
                has_pasp = true;
            }
            nread += head.size();
        }
        
        return size;
    }
    
    void show(int level) {
        print(level, "data_reference_index:%u\n", data_reference_index.little_endian_val());
        print(level, "width:%u\n", width.little_endian_val());
        print(level, "height:%u\n", height.little_endian_val());
        print(level, "horizresolution:%u\n", horizresolution.little_endian_val());
        print(level, "vertresolution:%u\n", vertresolution.little_endian_val());
        print(level, "frame_count:%u\n", frame_count.little_endian_val());
        print(level, "depth:%u\n", depth.little_endian_val());
        
        if (has_avcc) {
            print(level, "avcC\n");
            avcc_box.show(level + 1);
        }
        
        if (has_pasp) {
            print(level, "pasp\n");
            pasp_box.show(level + 1);
        }
    }
};

int read_es_size(int fd, uint32_t & size) {
    nbytes_t<uint8_t> value;
    size = 0;
    int i = 0;
    
    for (i = 0; i < 4; i++) {
        value.read(fd);
        
        size = (size << 7) + (value.i_val & 0x7F);
        if ((value.i_val & 0x80) == 0) {
            break;
        }
    }
    return i < 4 ? i + 1 : i;
}

// The following values are extracted from ISO 14496 Part 1 Table 5 -
// objectTypeIndication Values. Only values currently in use are included.
enum ObjectType {
    kForbidden = 0,
    kISO_14496_3 = 0x40,         // MPEG4 AAC
    kISO_13818_7_AAC_LC = 0x67,  // MPEG2 AAC-LC
    kAC3 = 0xa5,                 // AC3
    kEAC3 = 0xa6                 // EAC3 / Dolby Digital Plus
};


struct decoder_specific_info_t
{
    nbytes_t<uint8_t> tag;
    uint32_t size;
    uint8_t info[1024];
    
    int read(int fd, int isize) {
        int nread = 0;
        nread += tag.read(fd);
        nread += read_es_size(fd, size);
        nread += ::read(fd, info, size);
        //printf("decoder_specific_info_t size:%u\n", size);
        //printf("remain:%d\n", isize - nread);
        if (isize - nread > 0) { //0x06 tag, what?
            decoder_specific_info_t info_1;
            nread += info_1.read(fd, isize - nread);
        }
        return isize;
    }
};

struct decoder_config_desc_t 
{
    nbytes_t<uint8_t> tag;
    uint32_t size;
    nbytes_t<uint8_t> object_type;
    nbytes_t<uint64_t> dummy64;
    nbytes_t<uint32_t> dummy32;
    decoder_specific_info_t decoder_specific_info;
    
    int read(int fd, int isize) {
        int nread = 0;
        nread += tag.read(fd);
        nread += read_es_size(fd, size);
        //printf("decoder_config_desc_t size:%u\n", size);
        nread += object_type.read(fd);
        nread += dummy64.read(fd);
        nread += dummy32.read(fd);
        nread += decoder_specific_info.read(fd, isize - nread);
        
        return isize;
    }
    
    bool is_aac(void) const {
        return object_type.i_val == kISO_14496_3 || 
            kISO_13818_7_AAC_LC == object_type.i_val;
    }
};

//Element Stream Descriptors
struct esds_box_t 
{
    nbytes_t<uint32_t> version_flag;

    nbytes_t<uint8_t> ES_DescrTag;
    uint32_t Length_Field;
    nbytes_t<uint16_t> ES_ID;
    
    uint8_t streamDependenceFlag;//1bits
    uint8_t URL_Flag;//1bits
    uint8_t OCRstreamFlag;//1bits
    uint8_t streamPriority;//5bits;
    
    //steamDependenceFlag
    nbytes_t<uint16_t> dependsOn_ES_ID; 
    
    //URL_Flag
    nbytes_t<uint8_t> URLlength;
    char URLstring[1024];
    
    //OCRstreamFlag
    nbytes_t<uint16_t> OCR_ES_Id; 
    
    decoder_config_desc_t decoder_config_desc;
    
    int read(int fd, int isize) {
        int nread = 0;
        
        nread += version_flag.read(fd);
        nread += ES_DescrTag.read(fd);
        nread += read_es_size(fd, Length_Field);
        nread += ES_ID.read(fd);
        //printf("Length_Field:%u\n", Length_Field);
        nbytes_t<uint8_t> tmp;
        nread += tmp.read(fd);
        streamDependenceFlag = (tmp.i_val >> 7) & 0x01;
        URL_Flag = (tmp.i_val >> 6) & 0x01;
        OCRstreamFlag = (tmp.i_val >> 5) & 0x01;
        streamPriority = tmp.i_val & 0x1F;
        
        if (streamDependenceFlag) {
            nread += dependsOn_ES_ID.read(fd);
        }
        
        if (OCRstreamFlag) {
            nread += OCR_ES_Id.read(fd);
        }
        
        nread += decoder_config_desc.read(fd, isize - nread);
        
        //printf("remain:%d\n", isize - nread);
        return isize;
    }
    
    void show(int level) {
        print(level, "codec:%s\n", decoder_config_desc.is_aac() ? "AAC" : "Other");
    }
};

struct audio_sample_entry_t
{
    uint8_t reserved0[6];    //6bytes
    nbytes_t<uint16_t> data_reference_index;
    nbytes_t<uint32_t> reserved[2];
    nbytes_t<uint16_t> channelcount;
    nbytes_t<uint16_t> samplesize;
    nbytes_t<uint16_t> pre_defined;
    nbytes_t<uint16_t> reserved1;
    nbytes_t<uint32_t> samplerate;
    
    esds_box_t esds_box;
    bool has_esds;
    
    int read(int fd, int size) {
        int nread = 0;
        has_esds = false;
        nread += ::read(fd, reserved0, 6);
        nread += data_reference_index.read(fd);
        nread += reserved[0].read(fd);
        nread += reserved[1].read(fd);
        nread += channelcount.read(fd);
        nread += samplesize.read(fd);
        nread += pre_defined.read(fd);
        nread += reserved1.read(fd);
        nread += samplerate.read(fd);
        
//        if (channelcount.i_val == 0) {
//            channelcount.sz_val[1] = 2;
//        }
//        if (samplesize.i_val == 0) {
//            samplesize.sz_val[1] = 16;
//        }        
        //printf("off:%llx\n", lseek(fd, 0, SEEK_CUR));
        
        while (size - nread > 0) {
            boxhead_t head;
            head.read(fd);
            
            //printf("head:%s\n", head.type());
            if (strncmp(head.type(), "esds", 4) == 0) {
                nread += esds_box.read(fd, head.size() - head.head_bytes());
                has_esds = true;
            } else {
                //skip 
            }
            break;
        }
        
        return size;
    }
    
    void show(int level) {
        print(level, "channelcount:%u\n", channelcount.little_endian_val());
        print(level, "samplesize:%u\n", samplesize.little_endian_val());
        print(level, "samplerate:%u\n", (samplerate.little_endian_val() >> 16) & 0xFFFF);
        
        if (has_esds) {
            print(level, "esds\n");
            esds_box.show(level + 1);
        }
    }
};

struct hint_sample_entry_t
{
    int read(int fd, int size) {
        off_t cur_off = lseek(fd, size, SEEK_CUR);
        return size;
    }
};

struct sample_description_t
{
    nbytes_t<uint32_t> size;
    nbytes_t<uint32_t> type;
    union {
        visual_sample_entry_t video;
        audio_sample_entry_t audio;
        hint_sample_entry_t hint;
    } sample;
    int u_type;
    
    int read(int fd, int len) {
        int nread;
        
        nread = size.read(fd);
        nread = type.read(fd);

        //printf("type:%s\n", (const char *)type.value());
        if (strncmp((const char *)type.value(), "mp4a", 4) == 0) { //audio
            u_type = 0;
            sample.audio.read(fd, len - 8);
        } else if (strncmp((const char *)type.value(), "avc1", 4) == 0) { //video
            u_type = 1;
            sample.video.read(fd, len - 8);
        } else {    //hint, skip.
            u_type = 2;
            sample.hint.read(fd, len - 8);
        }
        
        return len;
    }
    
    void show(int level) {
        if (u_type == 0) {
            print(level, "mp4a\n");
            sample.audio.show(level + 1);
        } else if (u_type == 1) {
            print(level, "avc1\n");
            sample.video.show(level + 1);
        } else {
            print(level, "hint\n");
        }
    }
};

struct stsd_box_t {
    nbytes_t<uint32_t> version_flag;
    nbytes_t<uint32_t> number_of_entries;
    sample_description_t sample_description[10];
    
    int read(int fd, int size) {
        int nread = 0;
        
        nread += version_flag.read(fd);
        nread += number_of_entries.read(fd);
        for (int i = 0; i < number_of_entries.little_endian_val(); i++) {
            nread += sample_description[i].read(fd, size - nread);
        }
        
        return size;
    }
    
    void show(int level) {
        print(level, "number_of_entries:%u\n", number_of_entries.little_endian_val());
        for (int i = 0; i < number_of_entries.little_endian_val(); i++) {
            sample_description[i].show(level);
        }
    }
};

struct time2sample_t
{
    nbytes_t<uint32_t> sample_count;
    nbytes_t<uint32_t> sample_duration;
    
    int read(int fd, int size) {
        int nread = 0;
        nread += sample_count.read(fd);
        nread += sample_duration.read(fd);
        
        return size;
    }
    
    void show(int level) {
        print(level, "%u\t\t%u\n", 
              sample_count.little_endian_val(), 
              sample_duration.little_endian_val());
    }
};

struct stts_box_t {
    nbytes_t<uint32_t> version_flag;
    nbytes_t<uint32_t> nsamples;
    std::vector<time2sample_t> table;
    
    int read(int fd, int size) {
        int nread = 0;
        
        nread += version_flag.read(fd);
        nread += nsamples.read(fd);
        time2sample_t sample;
        for (int i = 0; i < nsamples.little_endian_val(); i++) {
            nread += sample.read(fd, size - nread);            
            table.push_back(sample);
        }
        return size;
    }
    
    void show(int level) {
        print(level, "nsamples:%u\n", nsamples.little_endian_val());
        print(level + 1, "sample_count\tsample_duration\n");
        for (int i = 0; i < nsamples.little_endian_val(); i++) {
            table[i].show(level + 1);
        }
    }
};

//sync sample atom
struct stss_box_t {
    nbytes_t<uint32_t> version_flag;
    nbytes_t<uint32_t> nsync_samples;
    
    typedef nbytes_t<uint32_t> sample_t;
    std::vector<sample_t> sync_samples;
    
    int read(int fd, int size) {
        int nread;
        
        nread += version_flag.read(fd);
        nread += nsync_samples.read(fd);
        
        sample_t sample;
        for (int i = 0; i < nsync_samples.little_endian_val(); i++) {
            nread += sample.read(fd);
            sync_samples.push_back(sample);
        }
        return size;
    }
    
    void show(int level) {
        print(level, "nsync-sample:%u\n", nsync_samples.little_endian_val());
        for (int i = 0; i < nsync_samples.little_endian_val(); i++) {
            print(level + 1, "%u\n", sync_samples[i].little_endian_val());
        }
    }
};

//CT(n) = DT(n) + CTTS(n)
//Composition Offset Atom, simliar to stts
struct ctts_box_t {
    nbytes_t<uint32_t> version_flag;
    nbytes_t<uint32_t> nsamples;
    std::vector<time2sample_t> table;
    
    int read(int fd, int size) {
        int nread = 0;
        
        nread += version_flag.read(fd);
        nread += nsamples.read(fd);
        time2sample_t sample;
        for (int i = 0; i < nsamples.little_endian_val(); i++) {
            nread += sample.read(fd, size - nread);            
            table.push_back(sample);
        }
        return size;
    }
    
    void show(int level) {
        print(level, "nsamples:%u\n", nsamples.little_endian_val());
        print(level + 1, "sample_count\tsample_duration\n");
        int nprint = nsamples.little_endian_val() > 10 ? 10 : nsamples.little_endian_val();
        for (int i = 0; i < nprint; i++) {
            table[i].show(level + 1);
        }
    }
};

struct sample_to_chunk_t
{
    nbytes_t<uint32_t> first_chunk;
    nbytes_t<uint32_t> samples_per_chunk;
    nbytes_t<uint32_t> sample_desc_id;
    
    int read(int fd, int size) {
        int nread = 0;
        
        nread += first_chunk.read(fd);
        nread += samples_per_chunk.read(fd);
        nread += sample_desc_id.read(fd);
        
        return size;
    }
    
    void show(int level) {
        print(level, "%u\t%u\t%u\n", 
              first_chunk.little_endian_val(),
              samples_per_chunk.little_endian_val(),
              sample_desc_id.little_endian_val());
    }
};

//sample-to-chunk atom
struct stsc_box_t {
    nbytes_t<uint32_t> version_flag;
    nbytes_t<uint32_t> nsample2chunks;
    std::vector<sample_to_chunk_t> sample2chunks;
    
    int read(int fd, int size) {
        int nread = 0;
        
        nread += version_flag.read(fd);
        nread += nsample2chunks.read(fd);
        
        sample_to_chunk_t sample;
        for (int i = 0; i < nsample2chunks.little_endian_val(); i++) {
            nread += sample.read(fd, size - nread);
            sample2chunks.push_back(sample);
        }
        return size;
    }
    
    void show(int level) {
        print(level, "chunk count:%u\n", nsample2chunks.little_endian_val());
        int nprint = nsample2chunks.little_endian_val() > 10 ? 10 : nsample2chunks.little_endian_val();
        
        print(level + 1, "first_chunk\tsamples_per_chunk\tsample_desc_id\n");
        for (int i = 0; i < nprint; i++) {
            sample2chunks[i].show(level + 1);
        }
    }
};

//stsz
struct stsz_box_t {
    nbytes_t<uint32_t> version_flag;
    nbytes_t<uint32_t> sample_size;
    nbytes_t<uint32_t> nsample;
    typedef nbytes_t<uint32_t> sample_t;
    std::vector<sample_t> samples;
    
    int read(int fd, int size) {
        int nread = 0;
        
        nread += version_flag.read(fd);
        nread += sample_size.read(fd);
        if (sample_size.little_endian_val() == 0) {
            nread += nsample.read(fd);
            
            sample_t sample;
            for (int i = 0; i < nsample.little_endian_val(); i++) {
                nread += sample.read(fd);
                samples.push_back(sample);
            }
        }
        return size;
    }
    
    void show(int level) {
        print(level, "sample_size:%u\n", sample_size.little_endian_val());
        print(level, "sample_table_size:%u\n", nsample.little_endian_val());
        print(level + 1, "sample size\n");
        int nprint = nsample.little_endian_val() > 10 ? 10 : nsample.little_endian_val();
        for (int i = 0; i < nprint; i++) {
            print(level + 1, "%u\n", samples[i].little_endian_val());
        }
    }
};

//Chunk Offset Atoms
struct stco_box_t {
    nbytes_t<uint32_t> version_flag;
    nbytes_t<uint32_t> table_size;
    typedef nbytes_t<uint32_t> table_cell_t;
    std::vector<table_cell_t> table;
    
    int read(int fd, int size) {
        int nread = 0;
        
        nread += version_flag.read(fd);
        nread += table_size.read(fd);
        
        table_cell_t cell;
        for (int i = 0; i < table_size.little_endian_val(); i++) {
            nread += cell.read(fd);
            table.push_back(cell);
        }
        return size;
    }
    
    void show(int level) {
        print(level, "Chunk Offset table size:%u\n", table_size.little_endian_val());
        print(level + 1, "sample offset\n");
        int nprint = table_size.little_endian_val() > 10 ? 10 : table_size.little_endian_val();
        for (int i = 0; i < nprint; i++) {
            print(level + 1, "%u\n", table[i].little_endian_val());
        }
    }
};

int mp4_read_ftyp_box(int fd, boxhead_t & ftyp, int level)
{
    //print(0, "ftyp box size:%08lx, headsize:%d\n", ftyp.size(), ftyp.head_bytes());
    print(level, "ftyp\n");
    
    ftyp_box_t ftyp_box;
    int data_bytes = ftyp.size() - ftyp.head_bytes();
    ftyp_box.read(fd, data_bytes);
    ftyp_box.show(level + 1);
    
    return 0;
}

int mp4_read_box_free(int fd, boxhead_t & hdr, int level) 
{
    print(level, "free\n");
    return 0;
}

int mp4_read_box_noop(int fd, boxhead_t & head, int level) 
{
    print(level, "read box:%s, size:%08lx, headsize:%d\n",
           head.type(), head.size(), head.head_bytes());
    
    int data_bytes = head.size() - head.head_bytes();
    if (data_bytes > 0) {
        off_t cur_off = lseek(fd, data_bytes, SEEK_CUR); //get current offset
    } else {
        print(level, "empty box\n");
    }
    
    return data_bytes;
}

int mp4_read_box_mvhd(int fd, boxhead_t & mvhd_hdr, int level) 
{
    //print(2, "read mvhd, size:%08lx\n", mvhd_hdr.size());
    print(level, "mvhd\n");
    int data_bytes = mvhd_hdr.size() - mvhd_hdr.head_bytes();
    mvhd_box_t mvhd_box;
    
    if (mvhd_box.read(fd, data_bytes) != data_bytes) {
        printf("read mvhd error\n");
        return -1;
    }
    mvhd_box.show(level + 1);
    return 0;
}

int mp4_read_box_tkhd(int fd, boxhead_t & hdr, int level) 
{
    print(level, "tkhd\n");
    tkhd_box_t box;
    int data_bytes = hdr.size() - hdr.head_bytes();
    if (box.read(fd, data_bytes) != data_bytes) {
        print(level, "read thkd error\n");
        return -1;
    }
    box.show(level + 1);
    
    return 0;
}

int mp4_read_box_edts(int fd, boxhead_t & hdr, int level) 
{
    print(level, "edts\n");
    elts_box_t box;
    int data_bytes = hdr.size() - hdr.head_bytes();
    if (data_bytes > 0) {
        boxhead_t eltshead;
        eltshead.read(fd);
        
        if (box.read(fd, data_bytes) != data_bytes) {
            print(level, "read edts error\n");
            return -1;
        }
        box.show(level + 1);
    }
    return 0;
}

int mp4_read_box_mdhd(int fd, boxhead_t & hdr, int level) 
{
    print(level, "mdhd\n");
    int data_bytes = hdr.size() - hdr.head_bytes();
    mdhd_box_t box;
    box.read(fd, data_bytes);
    box.show(level + 1);
    
    return 0;
}

int mp4_read_box_hdlr(int fd, boxhead_t & hdr, int level) 
{
    print(level, "hdlr\n");
    int data_bytes = hdr.size() - hdr.head_bytes();
    hdlr_box_t box;
    box.read(fd, data_bytes);
    box.show(level + 1);
    
    return 0;
}

int mp4_read_box_vmhd(int fd, boxhead_t & hdr, int level) 
{
    print(level, "vmhd\n");
    int data_bytes = hdr.size() - hdr.head_bytes();
    vmhd_box_t box;
    box.read(fd, data_bytes);
    box.show(level + 1);
    
    return 0;
}

int mp4_read_box_smhd(int fd, boxhead_t & hdr, int level) 
{
    print(level, "smhd\n");
    int data_bytes = hdr.size() - hdr.head_bytes();
    smhd_box_t box;
    box.read(fd, data_bytes);
    box.show(level + 1);
    
    return 0;
}

int mp4_read_box_dinf(int fd, boxhead_t & hdr, int level) 
{
    print(level, "dinf\n");
    int data_bytes = hdr.size() - hdr.head_bytes();
    
    if (data_bytes > 0) {
        boxhead_t drefhead;
        drefhead.read(fd);
        
        print(level + 1, "dref\n");
        dref_box_t box;
        box.read(fd, drefhead.size() - drefhead.head_bytes());
        box.show(level + 2);
    }
    
    return 0;
}

int mp4_read_box_stsd(int fd, boxhead_t & hdr, int level)
{
    print(level, "stsd\n");
    
    stsd_box_t box;
    box.read(fd, hdr.size() - hdr.head_bytes());
    box.show(level + 1);
    
    return 0;
}

//Time-to-sample atoms
int mp4_read_box_stts(int fd, boxhead_t & hdr, int level)
{
    print(level, "stts\n");
    
    stts_box_t box;
    box.read(fd, hdr.size() - hdr.head_bytes());
    box.show(level + 1);
    
    return 0;
}

//Time-to-sample atoms
int mp4_read_box_stss(int fd, boxhead_t & hdr, int level)
{
    print(level, "stts\n");
    
    stss_box_t box;
    box.read(fd, hdr.size() - hdr.head_bytes());
    box.show(level + 1);
    
    return 0;
}

//Sample-to-chunk atoms
int mp4_read_box_stsc(int fd, boxhead_t & hdr, int level)
{
    print(level, "stsc\n");
    
    stsc_box_t box;
    box.read(fd, hdr.size() - hdr.head_bytes());
    box.show(level + 1);
    
    return 0;
}

int mp4_read_box_stsz(int fd, boxhead_t & hdr, int level)
{
    print(level, "stsz\n");
    
    stsz_box_t box;
    box.read(fd, hdr.size() - hdr.head_bytes());
    box.show(level + 1);
    
    return 0;
}

int mp4_read_box_stco(int fd, boxhead_t & hdr, int level)
{
    print(level, "stco\n");
    
    stco_box_t box;
    box.read(fd, hdr.size() - hdr.head_bytes());
    box.show(level + 1);
    
    return 0;
}

int mp4_read_box_ctts(int fd, boxhead_t & hdr, int level)
{
    print(level, "ctts\n");
    
    ctts_box_t box;
    box.read(fd, hdr.size() - hdr.head_bytes());
    box.show(level + 1);
    
    return 0;
}

int mp4_read_box_stbl(int fd, boxhead_t & hdr, int level) 
{
    print(level, "stbl\n");
    long data_bytes = hdr.size() - hdr.head_bytes();
    long read_bytes = 0;
    
    boxhead_t head;
    while (true) {
        head.clear();
        if (head.read(fd) == 0) {
            print(level + 1, "mp4_read_box_mdia, read end of file\n");
            return -1;
        }
        
        if (!head.is_ok()) {
            print(level + 1, "read moov/trak/mdia box head error\n");
            return -1;
        }
        
        read_bytes += head.size();
        //print(level, "len:%lu, type:%s\n", head.size(), head.type());
        if (strcmp(head.type(), "stsd") == 0) {
            mp4_read_box_stsd(fd, head, level + 1);
        } else if (strcmp(head.type(), "stts") == 0) {
            mp4_read_box_stts(fd, head, level + 1);
        } else if (strcmp(head.type(), "stss") == 0) {
            mp4_read_box_stss(fd, head, level + 1);
        } else if (strcmp(head.type(), "stsc") == 0) {
            mp4_read_box_stsc(fd, head, level + 1);
        } else if (strcmp(head.type(), "stsz") == 0) {
            mp4_read_box_stsz(fd, head, level + 1);
        } else if (strcmp(head.type(), "stco") == 0) {
            mp4_read_box_stco(fd, head, level + 1);
        } else if (strcmp(head.type(), "ctts") == 0) {
            mp4_read_box_ctts(fd, head, level + 1);
        } else {
            mp4_read_box_noop(fd, head, level + 1);
        }
        
        //printf("stbl offset:%llx\n", lseek(fd, 0, SEEK_CUR));
        if (read_bytes == data_bytes) {
            //print(level, "read moov/trak/mdia/minf/stbl end\n");
            break;
        }
    }
    
    return 0;
}

int mp4_read_box_minf(int fd, boxhead_t & hdr, int level)
{
    print(level, "minf\n");
    long data_bytes = hdr.size() - hdr.head_bytes();
    long read_bytes = 0;
    
    boxhead_t head;
    while (true) {
        head.clear();
        if (head.read(fd) == 0) {
            print(level + 1, "mp4_read_box_mdia, read end of file\n");
            return -1;
        }
        
        if (!head.is_ok()) {
            print(level + 1, "read moov/trak/mdia box head error\n");
            return -1;
        }
        
        read_bytes += head.size();
        //print(level, "len:%08lx, type:%s\n", head.size(), head.type());
        if (strcmp(head.type(), "vmhd") == 0) {
            mp4_read_box_vmhd(fd, head, level + 1);
        } else if (strcmp(head.type(), "smhd") == 0) {
            mp4_read_box_smhd(fd, head, level + 1);
        } else if (strcmp(head.type(), "dinf") == 0) {
            mp4_read_box_dinf(fd, head, level + 1);
        } else if (strcmp(head.type(), "stbl") == 0) {
            mp4_read_box_stbl(fd, head, level + 1);
        } else {
            mp4_read_box_noop(fd, head, level + 1);
        }
        
        if (read_bytes == data_bytes) {
            //print(level, "read moov/trak/mdia/minf end\n");
            break;
        }
    }
    
    return 0;
}

int mp4_read_box_mdia(int fd, boxhead_t & hdr, int level) 
{
    long data_bytes = hdr.size() - hdr.head_bytes();
    long read_bytes = 0;
    
    print(level, "mdia\n");
    boxhead_t head;
    while (true) {
        head.clear();
        if (head.read(fd) == 0) {
            print(level + 1, "mp4_read_box_mdia, read end of file\n");
            return -1;
        }
        
        if (!head.is_ok()) {
            print(level + 1, "read moov/trak/mdia box head error\n");
            return -1;
        }
        
        read_bytes += head.size();
        //print(level, "len:%08lx, type:%s\n", head.size(), head.type());
        
        if (strcmp(head.type(), "mdhd") == 0) {
            mp4_read_box_mdhd(fd, head, level + 1);
        } else if (strcmp(head.type(), "hdlr") == 0) {
            mp4_read_box_hdlr(fd, head, level + 1);
        } else if (strcmp(head.type(), "minf") == 0) {
            mp4_read_box_minf(fd, head, level + 1);
        } else {
            mp4_read_box_noop(fd, head, level + 1);
        }
        
        if (read_bytes == data_bytes) {
            //print(level, "read moov/trak/mdia end\n");
            break;
        }
    }
    
    return 0;
}

int mp4_read_box_trak(int fd, boxhead_t & trak_hdr, int level) 
{
    long data_bytes = trak_hdr.size() - trak_hdr.head_bytes();
    long read_bytes = 0;
    
    print(level, "trak\n");
    boxhead_t head;
    while (true) {
        head.clear();
        if (head.read(fd) == 0) {
            print(level + 1, "mp4_read_box_trak, read end of file\n");
            return -1;
        }
        
        if (!head.is_ok()) {
            print(level + 1, "read moov/trak internal box head error\n");
            return -1;
        }
        
        read_bytes += head.size();
        //print(2, "len:%08lx, type:%s\n", head.size(), head.type());
//        mp4_read_box_noop(fd, head, level + 1);
        
        if (strcmp(head.type(), "tkhd") == 0) {
            mp4_read_box_tkhd(fd, head, level + 1);
        } else if (strcmp(head.type(), "edts") == 0) {  //list
            mp4_read_box_edts(fd, head, level + 1);
        } else if (strcmp(head.type(), "mdia") == 0) {
            mp4_read_box_mdia(fd, head, level + 1);
        } else {
            mp4_read_box_noop(fd, head, level + 1);
        }
    
        if (read_bytes == data_bytes) {
            //print(level, "read trak end\n");
            break;
        }
    }
    
    return 0;
}

int mp4_read_box_moov(int fd, boxhead_t & moov, int level)
{
    //print(0, "read moov, size:%08lx\n", moov.size());
    print(level, "moov\n");
    boxhead_t head;
    long data_bytes = moov.size() - moov.head_bytes();
    long read_bytes = 0;
    
    while (true) {
        head.clear();
        if (head.read(fd) == 0) {
            print(level, "mp4_read_box_moov, read end of file\n");
            break;
        }
        
        if (!head.is_ok()) {
            print(level, "read moov internal box head error\n");
            return -1;
        }
        
        read_bytes += head.size();
        //print(1, "len:%08lx, type:%s\n", head.size(), head.type());
        
        if (strcmp(head.type(), "mvhd") == 0) {
            mp4_read_box_mvhd(fd, head, level + 1);
        } else if (strcmp(head.type(), "trak") == 0) {
            mp4_read_box_trak(fd, head, level + 1);
        } else {
            mp4_read_box_noop(fd, head, level + 1);
        }
        
        if (read_bytes == data_bytes) {
            //print(level, "read moov end\n");
            break;
        }
    }
    //printf("read:%ld, data:%ld\n", read_bytes, data_bytes);
    return 0;
}

int analyse_mp4(int fd, int level)
{
    boxhead_t head;
    off_t fsize = 0;
    
    while (true) {
        head.clear();
        if (head.read(fd) == 0) {
            printf("read end of file\n");
            break;
        }
        if (!head.is_ok()) {
            printf("read box head error\n");
            break;
        }
        printf("--------------------------\n");
        fsize += head.size();
        //printf("len:%08lx, type:%s\n", head.size(), head.type());
        
        if (strcmp(head.type(), "ftyp") == 0) {
            if (mp4_read_ftyp_box(fd, head, level + 1) < 0) {
                printf("read ftyp box error\n");
                break;
            }
        } else if (strcmp(head.type(), "free") == 0) {
            if (mp4_read_box_free(fd, head, level + 1) < 0) {
                printf("read free box error\n");
                break;
            }
        } else if (strcmp(head.type(), "moov") == 0) {
            if (mp4_read_box_moov(fd, head, level + 1) < 0) {
                printf("read moov box error\n");
                break;
            }
        } else {
            if (mp4_read_box_noop(fd, head, level + 1) < 0) {
                printf("read other box error\n");
                break;
            }
        }
    }
    printf("--------------------------\n");
    printf("filesize:%lld\n", fsize);
    
    return 0;
}

int main(int argc, char * argv[]) 
{
    if (argc < 2) {
        printf("input mp4 file name\n");
        return -1;
    }
    
    const char * filename = argv[1];
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        printf("open file:%s error<%s>\n", filename, strerror(errno));
        return -1;
    }
    printf("open mp4file:%s\n", filename);
    
    analyse_mp4(fd, 0);
    
    close(fd);
    return 0;
}
