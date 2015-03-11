// Copyright (C) 2015 Vincent Lejeune

#ifndef VAOMANAGER_HPP
#define VAOMANAGER_HPP

#include <Core/IMeshBuffer.h>
#include <Core/Singleton.h>
#include <vector>
#include <map>
#include <unordered_map>
#include <tuple>

enum InstanceType
{
    InstanceTypeDualTex,
    InstanceTypeThreeTex,
    InstanceTypeShadow,
    InstanceTypeRSM,
    InstanceTypeGlow,
    InstanceTypeCount,
};

#ifdef WIN32
#pragma pack(push, 1)
#endif
struct InstanceDataShadow
{
    struct
    {
        float X;
        float Y;
        float Z;
    } Origin;
    struct
    {
        float X;
        float Y;
        float Z;
    } Orientation;
    struct
    {
        float X;
        float Y;
        float Z;
    } Scale;
    uint64_t Texture;
#ifdef WIN32
};
#else
} __attribute__((packed));
#endif

struct InstanceDataRSM
{
    struct
    {
        float X;
        float Y;
        float Z;
    } Origin;
    struct
    {
        float X;
        float Y;
        float Z;
    } Orientation;
    struct
    {
        float X;
        float Y;
        float Z;
    } Scale;
    uint64_t Texture;
#ifdef WIN32
};
#else
} __attribute__((packed));
#endif

struct InstanceDataDualTex
{
    struct
    {
        float X;
        float Y;
        float Z;
    } Origin;
    struct
    {
        float X;
        float Y;
        float Z;
    } Orientation;
    struct
    {
        float X;
        float Y;
        float Z;
    } Scale;
    uint64_t Texture;
    uint64_t SecondTexture;
#ifdef WIN32
};
#else
} __attribute__((packed));
#endif

struct InstanceDataThreeTex
{
    struct
    {
        float X;
        float Y;
        float Z;
    } Origin;
    struct
    {
        float X;
        float Y;
        float Z;
    } Orientation;
    struct
    {
        float X;
        float Y;
        float Z;
    } Scale;
    uint64_t Texture;
    uint64_t SecondTexture;
    uint64_t ThirdTexture;
#ifdef WIN32
};
#else
} __attribute__((packed));
#endif

struct GlowInstanceData
{
    struct
    {
        float X;
        float Y;
        float Z;
    } Origin;
    struct
    {
        float X;
        float Y;
        float Z;
    } Orientation;
    struct
    {
        float X;
        float Y;
        float Z;
    } Scale;
    unsigned Color;
#ifdef WIN32
};
#else
} __attribute__((packed));
#endif

struct SkinnedVertexData
{
    int index0;
    float weight0;
    int index1;
    float weight1;
    int index2;
    float weight2;
    int index3;
    float weight3;
#ifdef WIN32
};
#else
} __attribute__((packed));
#endif

#ifdef WIN32
#pragma pack(pop)
#endif

template<typename VT>
struct VertexAttribBinder
{
public:
    static void bind();
};

/** Wrapper around opengl buffer. It strongly types underlying data
*  and automatically makes the buffer persistent if buffer storage is available.
*  /tparam Data is the base type of the array
*  /tparam BufferType is the buffer target (used in bindBuffer)
*/
template<typename Data, GLenum BufferType>
class ArrayBuffer
{
private:
    GLuint buffer;
    size_t realSize;
    size_t advertisedSize;

    Data *Pointer;
public:
    ArrayBuffer() : buffer(0), realSize(0), advertisedSize(0), Pointer(0) { }

    ~ArrayBuffer()
    {
        if (buffer)
            glDeleteBuffers(1, &buffer);
    }

    void resizeBufferIfNecessary(size_t requestedSize)
    {
        if (requestedSize >= realSize)
        {
            while (requestedSize >= realSize)
                realSize = 2 * realSize + 1;
            GLuint newVBO;
            glGenBuffers(1, &newVBO);
            glBindBuffer(BufferType, newVBO);
/*            if (CVS->supportsAsyncInstanceUpload())
            {
                glBufferStorage(BufferType, realSize * sizeof(Data), 0, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
                Pointer = static_cast<Data *>(glMapBufferRange(BufferType, 0, realSize * sizeof(Data), GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT));
            }
            else*/
                glBufferData(BufferType, realSize * sizeof(Data), 0, GL_DYNAMIC_DRAW);

            if (buffer)
            {
                // Copy old data
                GLuint oldVBO = buffer;
                glBindBuffer(GL_COPY_WRITE_BUFFER, newVBO);
                glBindBuffer(GL_COPY_READ_BUFFER, oldVBO);
                glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, advertisedSize * sizeof(Data));
                glDeleteBuffers(1, &oldVBO);
            }
            buffer = newVBO;
        }
        advertisedSize = requestedSize;
    }

    Data *getPointer()
    {
        return Pointer;
    }

    size_t getSize() const
    {
        return advertisedSize;
    }

    GLuint getBuffer(size_t id)
    {
        //assert(id == 0);
        return buffer;
    }

    void bindAttrib()
    {
        glBindBuffer(BufferType, buffer);
        VertexAttribBinder<Data>::bind();
    }

    void append(size_t count, void *ptr)
    {
        size_t from = advertisedSize;
        resizeBufferIfNecessary(from + count);
/*        if (CVS->supportsAsyncInstanceUpload())
        {
            void *tmp = (char*)Pointer + from * sizeof(Data);
            memcpy(tmp, ptr, count * sizeof(Data));
        }
        else*/
        {
            glBindBuffer(BufferType, buffer);
            glBufferSubData(BufferType, from * sizeof(Data), count * sizeof(Data), ptr);
        }
    }
};

/*template<typename Data>
class InstanceBuffer : public Singleton<InstanceBuffer<Data> >, public ArrayBuffer <Data, GL_ARRAY_BUFFER>
{
public:
    InstanceBuffer() : ArrayBuffer<Data, GL_ARRAY_BUFFER>()
    {
        ArrayBuffer<Data, GL_ARRAY_BUFFER>::resizeBufferIfNecessary(10000);
    }
};*/

template<typename VT>
struct InstanceAttribBinder
{
public:
    static void bind();
};

/** Set of buffers describing a formatted (ie with bound attributes) vertex storage.
*  /tparam S3DVertexFormat is the vertex type
*  /tparam AppendedData is used for vertex "annotation" in companion buffer (like skinning weight)
*/
template <typename S3DVertexFormat, typename ...AppendedData>
class FormattedVertexStorage : public std::tuple<ArrayBuffer<S3DVertexFormat, GL_ARRAY_BUFFER>, ArrayBuffer<AppendedData, GL_ARRAY_BUFFER>...>
{
private:
    template<typename TupleType, int N>
    struct BindAttrib_impl
    {
        static void exec(TupleType &tp)
        {
            const int Idx = std::tuple_size<TupleType>::value - N;
            std::get<Idx>(tp).bindAttrib();
            BindAttrib_impl<TupleType, N - 1>::exec(tp);
        }
    };
    template<typename TupleType>
    struct BindAttrib_impl<TupleType, 0>
    {
        static void exec(const TupleType &tp)
        { }
    };

    typedef std::tuple<ArrayBuffer<S3DVertexFormat, GL_ARRAY_BUFFER>, ArrayBuffer<AppendedData, GL_ARRAY_BUFFER>...> BaseType;

    template<int N>
    void append_impl(size_t count)
    {
        static_assert(N == std::tuple_size<BaseType>::value, "Wrong count");
    }

    template<int N, typename ... VoidPtrType>
    void append_impl(size_t count, void *ptr, VoidPtrType *...additionalData)
    {
        std::get<N>(*this).append(count, ptr);
        append_impl<N + 1>(count, additionalData...);
    }
public:
    typedef typename S3DVertexFormat VertexFormat;

    size_t getSize() const
    {
        return std::get<0>(*this).getSize();
    }

    void bindAttrib()
    {
        BindAttrib_impl<BaseType, std::tuple_size<BaseType>::value>::exec(*this);
    }

    template<typename ... VoidPtrType>
    void append(size_t count, void *ptr, VoidPtrType *...additionalData)
    {
        append_impl<0>(count, ptr, additionalData...);
    }
};

static void SetVertexAttrib_impl(size_t s)
{
    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, s, 0);
    glVertexAttribDivisorARB(7, 1);
    glEnableVertexAttribArray(8);
    glVertexAttribPointer(8, 3, GL_FLOAT, GL_FALSE, s, (GLvoid*)(3 * sizeof(float)));
    glVertexAttribDivisorARB(8, 1);
    glEnableVertexAttribArray(9);
    glVertexAttribPointer(9, 3, GL_FLOAT, GL_FALSE, s, (GLvoid*)(6 * sizeof(float)));
    glVertexAttribDivisorARB(9, 1);
}



template<>
struct InstanceAttribBinder < InstanceDataShadow >
{
public:
    static void SetVertexAttrib()
    {
        SetVertexAttrib_impl(sizeof(InstanceDataShadow));
        glEnableVertexAttribArray(10);
        glVertexAttribIPointer(10, 2, GL_UNSIGNED_INT, sizeof(InstanceDataShadow), (GLvoid*)(9 * sizeof(float)));
        glVertexAttribDivisorARB(10, 1);
    }
};

template<>
struct InstanceAttribBinder<InstanceDataRSM>
{
public:
    static void SetVertexAttrib()
    {
        SetVertexAttrib_impl(sizeof(InstanceDataRSM));
        glEnableVertexAttribArray(10);
        glVertexAttribIPointer(10, 2, GL_UNSIGNED_INT, sizeof(InstanceDataRSM), (GLvoid*)(9 * sizeof(float)));
        glVertexAttribDivisorARB(10, 1);
    }
};

template<>
struct InstanceAttribBinder<InstanceDataDualTex>
{
public:
    static void SetVertexAttrib()
    {
        SetVertexAttrib_impl(sizeof(InstanceDataDualTex));
        glEnableVertexAttribArray(10);
        glVertexAttribIPointer(10, 2, GL_UNSIGNED_INT, sizeof(InstanceDataDualTex), (GLvoid*)(9 * sizeof(float)));
        glVertexAttribDivisorARB(10, 1);
        glEnableVertexAttribArray(11);
        glVertexAttribIPointer(11, 2, GL_UNSIGNED_INT, sizeof(InstanceDataDualTex), (GLvoid*)(9 * sizeof(float) + 2 * sizeof(unsigned)));
        glVertexAttribDivisorARB(11, 1);
    }
};

template<>
struct InstanceAttribBinder<InstanceDataThreeTex>
{
public:
    static void SetVertexAttrib()
    {
        SetVertexAttrib_impl(sizeof(InstanceDataThreeTex));
        glEnableVertexAttribArray(10);
        glVertexAttribIPointer(10, 2, GL_UNSIGNED_INT, sizeof(InstanceDataThreeTex), (GLvoid*)(9 * sizeof(float)));
        glVertexAttribDivisorARB(10, 1);
        glEnableVertexAttribArray(11);
        glVertexAttribIPointer(11, 2, GL_UNSIGNED_INT, sizeof(InstanceDataThreeTex), (GLvoid*)(9 * sizeof(float) + 2 * sizeof(unsigned)));
        glVertexAttribDivisorARB(11, 1);
        glEnableVertexAttribArray(13);
        glVertexAttribIPointer(13, 2, GL_UNSIGNED_INT, sizeof(InstanceDataThreeTex), (GLvoid*)(9 * sizeof(float) + 4 * sizeof(unsigned)));
        glVertexAttribDivisorARB(13, 1);
    }
};

template<>
struct InstanceAttribBinder<GlowInstanceData>
{
public:
    static void SetVertexAttrib()
    {
        SetVertexAttrib_impl(sizeof(GlowInstanceData));
        glEnableVertexAttribArray(12);
        glVertexAttribPointer(12, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(GlowInstanceData), (GLvoid*)(9 * sizeof(float)));
        glVertexAttribDivisorARB(12, 1);
    }
};

/** Linear storage for all meshes with the same vertex format.
*  Should be used for "static" (ie with immutable triangle count) mesh.
*  Individual mesh can't be removed, the whole buffer need to be discarded.
*  The instance buffer are automatically rebound if necessary.
*  /tparam VBO is FormattedVertex type used for all mesh
*/
template<typename VBO>
class VertexArrayObject : public Singleton<VertexArrayObject<VBO> >
{
private:
    GLuint vao;
    std::unordered_map<irr::scene::IMeshBuffer<typename VBO::VertexFormat>*, std::pair<GLuint, GLuint> > mappedBaseVertexBaseIndex;

    template<typename... VertexAnnotation>
    void append(irr::scene::IMeshBuffer<typename VBO::VertexFormat> *mb, VertexAnnotation *...SkinnedData)
    {
        size_t old_vtx_cnt = vbo.getSize();
        size_t old_idx_cnt = ibo.getSize();

        vbo.append(mb->getVertexCount(), mb->getVertices(), SkinnedData...);
        ibo.append(mb->getIndexCount(), mb->getIndices());

        mappedBaseVertexBaseIndex[mb] = std::make_pair(old_vtx_cnt, old_idx_cnt * sizeof(uint16_t));
    }

    void regenerateVAO()
    {
        glDeleteVertexArrays(1, &vao);
        vao = getNewVAO();
        glBindVertexArray(0);
    }

    void regenerateInstanceVao()
    {
/*        glDeleteVertexArrays(1, &vao_instanceDualTex);
        vao_instanceDualTex = getNewVAO();
        glBindBuffer(GL_ARRAY_BUFFER, InstanceBuffer<InstanceDataDualTex>::getInstance()->getBuffer(0));
        InstanceAttribBinder<InstanceDataDualTex>::SetVertexAttrib();

        glDeleteVertexArrays(1, &vao_instanceThreeTex);
        vao_instanceThreeTex = getNewVAO();
        glBindBuffer(GL_ARRAY_BUFFER, InstanceBuffer<InstanceDataThreeTex>::getInstance()->getBuffer(0));
        InstanceAttribBinder<InstanceDataThreeTex>::SetVertexAttrib();

        glDeleteVertexArrays(1, &vao_instanceShadow);
        vao_instanceShadow = getNewVAO();
        glBindBuffer(GL_ARRAY_BUFFER, InstanceBuffer<InstanceDataShadow>::getInstance()->getBuffer(0));
        InstanceAttribBinder<InstanceDataShadow>::SetVertexAttrib();

        glDeleteVertexArrays(1, &vao_instanceRSM);
        vao_instanceRSM = getNewVAO();
        glBindBuffer(GL_ARRAY_BUFFER, InstanceBuffer<InstanceDataRSM>::getInstance()->getBuffer(0));
        InstanceAttribBinder<InstanceDataRSM>::SetVertexAttrib();

        glDeleteVertexArrays(1, &vao_instanceGlowData);
        vao_instanceGlowData = getNewVAO();
        glBindBuffer(GL_ARRAY_BUFFER, InstanceBuffer<GlowInstanceData>::getInstance()->getBuffer(0));
        InstanceAttribBinder<GlowInstanceData>::SetVertexAttrib();

        glBindVertexArray(0);*/
    }
public:
    GLuint vao_instanceDualTex, vao_instanceThreeTex, vao_instanceShadow, vao_instanceRSM, vao_instanceGlowData;

    VBO vbo;
    ArrayBuffer<uint16_t, GL_ELEMENT_ARRAY_BUFFER> ibo;

    VertexArrayObject()
    {
        glGenVertexArrays(1, &vao);
        glGenVertexArrays(1, &vao_instanceDualTex);
        glGenVertexArrays(1, &vao_instanceThreeTex);
        glGenVertexArrays(1, &vao_instanceShadow);
        glGenVertexArrays(1, &vao_instanceRSM);
        glGenVertexArrays(1, &vao_instanceGlowData);
    }

    ~VertexArrayObject()
    {
        glDeleteVertexArrays(1, &vao);
        glDeleteVertexArrays(1, &vao_instanceDualTex);
        glDeleteVertexArrays(1, &vao_instanceThreeTex);
        glDeleteVertexArrays(1, &vao_instanceShadow);
        glDeleteVertexArrays(1, &vao_instanceRSM);
        glDeleteVertexArrays(1, &vao_instanceGlowData);
    }

    GLuint getNewVAO()
    {
        GLuint newvao;
        glGenVertexArrays(1, &newvao);
        glBindVertexArray(newvao);
        vbo.bindAttrib();
        ibo.bindAttrib();
        return newvao;
    }

    template<typename... VertexAnnotation>
    std::pair<unsigned, unsigned> getBase(irr::scene::IMeshBuffer<typename VBO::VertexFormat> *mb, VertexAnnotation *...ptr)
    {
        if (mappedBaseVertexBaseIndex.find(mb) == mappedBaseVertexBaseIndex.end())
        {
            append(mb, ptr...);
            regenerateVAO();
            regenerateInstanceVao();
        }
        return mappedBaseVertexBaseIndex[mb];
    }

    bool isEmpty() const
    {
        return ibo.getSize() == 0;
    }

    GLuint getVAO()
    {
        return vao;
    }
};

#endif
