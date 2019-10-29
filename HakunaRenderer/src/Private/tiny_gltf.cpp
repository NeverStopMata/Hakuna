#include "tiny_gltf.h"
#include <algorithm>
//#include <cassert>
#ifndef TINYGLTF_NO_FS
#include <fstream>
#endif
#include <sstream>
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#ifdef __clang__
// Disable some warnings for external files.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#pragma clang diagnostic ignored "-Wpadded"
#pragma clang diagnostic ignored "-Wc++98-compat"
#pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
#pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
#pragma clang diagnostic ignored "-Wswitch-enum"
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#pragma clang diagnostic ignored "-Wweak-vtables"
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#if __has_warning("-Wdouble-promotion")
#pragma clang diagnostic ignored "-Wdouble-promotion"
#endif
#if __has_warning("-Wcomma")
#pragma clang diagnostic ignored "-Wcomma"
#endif
#if __has_warning("-Wzero-as-null-pointer-constant")
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif
#if __has_warning("-Wcast-qual")
#pragma clang diagnostic ignored "-Wcast-qual"
#endif
#if __has_warning("-Wmissing-variable-declarations")
#pragma clang diagnostic ignored "-Wmissing-variable-declarations"
#endif
#if __has_warning("-Wmissing-prototypes")
#pragma clang diagnostic ignored "-Wmissing-prototypes"
#endif
#if __has_warning("-Wcast-align")
#pragma clang diagnostic ignored "-Wcast-align"
#endif
#if __has_warning("-Wnewline-eof")
#pragma clang diagnostic ignored "-Wnewline-eof"
#endif
#endif

#include "json.hpp"

#ifdef TINYGLTF_ENABLE_DRACO
#include "draco/core/decoder_buffer.h"
#include "draco/compression/decode.h"
#endif

#ifndef TINYGLTF_NO_STB_IMAGE
#include "stb_image.h"
#endif

#ifndef TINYGLTF_NO_STB_IMAGE_WRITE
#include "stb_image_write.h"
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#ifdef _WIN32

// issue 143.
// Define NOMINMAX to avoid min/max defines,
// but undef it after included windows.h
#ifndef NOMINMAX
#define TINYGLTF_INTERNAL_NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#define TINYGLTF_INTERNAL_WIN32_LEAN_AND_MEAN
#endif
#include <windows.h> // include API for expanding a file path

#ifdef TINYGLTF_INTERNAL_WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif

#if defined(TINYGLTF_INTERNAL_NOMINMAX)
#undef NOMINMAX
#endif

#elif !defined(__ANDROID__)
#include <wordexp.h>
#endif

#if defined(__sparcv9)
// Big endian
#else
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) || MINIZ_X86_OR_X64_CPU
#define TINYGLTF_LITTLE_ENDIAN 1
#endif
#endif

using nlohmann::json;

#ifdef __APPLE__
#include "TargetConditionals.h"
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++98-compat"
#endif

namespace tinygltf {

// Equals function for Value, for recursivity
static bool Equals(const tinygltf::Value &one, const tinygltf::Value &other) {
  if (one.Type() != other.Type()) return false;

  switch (one.Type()) {
    case NULL_TYPE:
      return true;
    case BOOL_TYPE:
      return one.Get<bool>() == other.Get<bool>();
    case NUMBER_TYPE:
      return TINYGLTF_DOUBLE_EQUAL(one.Get<double>(), other.Get<double>());
    case INT_TYPE:
      return one.Get<int>() == other.Get<int>();
    case OBJECT_TYPE: {
      auto oneObj = one.Get<tinygltf::Value::Object>();
      auto otherObj = other.Get<tinygltf::Value::Object>();
      if (oneObj.size() != otherObj.size()) return false;
      for (auto &it : oneObj) {
        auto otherIt = otherObj.find(it.first);
        if (otherIt == otherObj.end()) return false;

        if (!Equals(it.second, otherIt->second)) return false;
      }
      return true;
    }
    case ARRAY_TYPE: {
      if (one.Size() != other.Size()) return false;
      for (int i = 0; i < int(one.Size()); ++i)
        if (Equals(one.Get(i), other.Get(i))) return false;
      return true;
    }
    case STRING_TYPE:
      return one.Get<std::string>() == other.Get<std::string>();
    case BINARY_TYPE:
      return one.Get<std::vector<unsigned char> >() ==
             other.Get<std::vector<unsigned char> >();
    default: {
      // unhandled type
      return false;
    }
  }
}

// Equals function for std::vector<double> using TINYGLTF_DOUBLE_EPSILON
static bool Equals(const std::vector<double> &one,
                   const std::vector<double> &other) {
  if (one.size() != other.size()) return false;
  for (int i = 0; i < int(one.size()); ++i) {
    if (!TINYGLTF_DOUBLE_EQUAL(one[size_t(i)], other[size_t(i)])) return false;
  }
  return true;
}

bool Accessor::operator==(const Accessor &other) const {
  return this->bufferView == other.bufferView &&
         this->byteOffset == other.byteOffset &&
         this->componentType == other.componentType &&
         this->count == other.count && this->extras == other.extras &&
         Equals(this->maxValues, other.maxValues) &&
         Equals(this->minValues, other.minValues) && this->name == other.name &&
         this->normalized == other.normalized && this->type == other.type;
}
bool Animation::operator==(const Animation &other) const {
  return this->channels == other.channels && this->extras == other.extras &&
         this->name == other.name && this->samplers == other.samplers;
}
bool AnimationChannel::operator==(const AnimationChannel &other) const {
  return this->extras == other.extras &&
         this->target_node == other.target_node &&
         this->target_path == other.target_path &&
         this->sampler == other.sampler;
}
bool AnimationSampler::operator==(const AnimationSampler &other) const {
  return this->extras == other.extras && this->input == other.input &&
         this->interpolation == other.interpolation &&
         this->output == other.output;
}
bool Asset::operator==(const Asset &other) const {
  return this->copyright == other.copyright &&
         this->extensions == other.extensions && this->extras == other.extras &&
         this->generator == other.generator &&
         this->minVersion == other.minVersion && this->version == other.version;
}
bool Buffer::operator==(const Buffer &other) const {
  return this->data == other.data && this->extras == other.extras &&
         this->name == other.name && this->uri == other.uri;
}
bool BufferView::operator==(const BufferView &other) const {
  return this->buffer == other.buffer && this->byteLength == other.byteLength &&
         this->byteOffset == other.byteOffset &&
         this->byteStride == other.byteStride && this->name == other.name &&
         this->target == other.target && this->extras == other.extras &&
         this->dracoDecoded == other.dracoDecoded;
}
bool Camera::operator==(const Camera &other) const {
  return this->name == other.name && this->extensions == other.extensions &&
         this->extras == other.extras &&
         this->orthographic == other.orthographic &&
         this->perspective == other.perspective && this->type == other.type;
}
bool Image::operator==(const Image &other) const {
  return this->bufferView == other.bufferView &&
         this->component == other.component && this->extras == other.extras &&
         this->height == other.height && this->image == other.image &&
         this->mimeType == other.mimeType && this->name == other.name &&
         this->uri == other.uri && this->width == other.width;
}
bool Light::operator==(const Light &other) const {
  return Equals(this->color, other.color) && this->name == other.name &&
         this->type == other.type;
}
bool Material::operator==(const Material &other) const {
  return this->additionalValues == other.additionalValues &&
         this->extensions == other.extensions && this->extras == other.extras &&
         this->name == other.name && this->values == other.values;
}
bool Mesh::operator==(const Mesh &other) const {
  return this->extensions == other.extensions && this->extras == other.extras &&
         this->name == other.name && this->primitives == other.primitives &&
         this->targets == other.targets && Equals(this->weights, other.weights);
}
bool Model::operator==(const Model &other) const {
  return this->accessors == other.accessors &&
         this->animations == other.animations && this->asset == other.asset &&
         this->buffers == other.buffers &&
         this->bufferViews == other.bufferViews &&
         this->cameras == other.cameras &&
         this->defaultScene == other.defaultScene &&
         this->extensions == other.extensions &&
         this->extensionsRequired == other.extensionsRequired &&
         this->extensionsUsed == other.extensionsUsed &&
         this->extras == other.extras && this->images == other.images &&
         this->lights == other.lights && this->materials == other.materials &&
         this->meshes == other.meshes && this->nodes == other.nodes &&
         this->samplers == other.samplers && this->scenes == other.scenes &&
         this->skins == other.skins && this->textures == other.textures;
}
bool Node::operator==(const Node &other) const {
  return this->camera == other.camera && this->children == other.children &&
         this->extensions == other.extensions && this->extras == other.extras &&
         Equals(this->matrix, other.matrix) && this->mesh == other.mesh &&
         this->name == other.name && Equals(this->rotation, other.rotation) &&
         Equals(this->scale, other.scale) && this->skin == other.skin &&
         Equals(this->translation, other.translation) &&
         Equals(this->weights, other.weights);
}
bool OrthographicCamera::operator==(const OrthographicCamera &other) const {
  return this->extensions == other.extensions && this->extras == other.extras &&
         TINYGLTF_DOUBLE_EQUAL(this->xmag, other.xmag) &&
         TINYGLTF_DOUBLE_EQUAL(this->ymag, other.ymag) &&
         TINYGLTF_DOUBLE_EQUAL(this->zfar, other.zfar) &&
         TINYGLTF_DOUBLE_EQUAL(this->znear, other.znear);
}
bool Parameter::operator==(const Parameter &other) const {
  if (this->bool_value != other.bool_value ||
      this->has_number_value != other.has_number_value)
    return false;

  if (!TINYGLTF_DOUBLE_EQUAL(this->number_value, other.number_value))
    return false;

  if (this->json_double_value.size() != other.json_double_value.size())
    return false;
  for (auto &it : this->json_double_value) {
    auto otherIt = other.json_double_value.find(it.first);
    if (otherIt == other.json_double_value.end()) return false;

    if (!TINYGLTF_DOUBLE_EQUAL(it.second, otherIt->second)) return false;
  }

  if (!Equals(this->number_array, other.number_array)) return false;

  if (this->string_value != other.string_value) return false;

  return true;
}
bool PerspectiveCamera::operator==(const PerspectiveCamera &other) const {
  return TINYGLTF_DOUBLE_EQUAL(this->aspectRatio, other.aspectRatio) &&
         this->extensions == other.extensions && this->extras == other.extras &&
         TINYGLTF_DOUBLE_EQUAL(this->yfov, other.yfov) &&
         TINYGLTF_DOUBLE_EQUAL(this->zfar, other.zfar) &&
         TINYGLTF_DOUBLE_EQUAL(this->znear, other.znear);
}
bool Primitive::operator==(const Primitive &other) const {
  return this->attributes == other.attributes && this->extras == other.extras &&
         this->indices == other.indices && this->material == other.material &&
         this->mode == other.mode && this->targets == other.targets;
}
bool Sampler::operator==(const Sampler &other) const {
  return this->extras == other.extras && this->magFilter == other.magFilter &&
         this->minFilter == other.minFilter && this->name == other.name &&
         this->wrapR == other.wrapR && this->wrapS == other.wrapS &&
         this->wrapT == other.wrapT;
}
bool Scene::operator==(const Scene &other) const {
  return this->extensions == other.extensions && this->extras == other.extras &&
         this->name == other.name && this->nodes == other.nodes;
  ;
}
bool Skin::operator==(const Skin &other) const {
  return this->inverseBindMatrices == other.inverseBindMatrices &&
         this->joints == other.joints && this->name == other.name &&
         this->skeleton == other.skeleton;
}
bool Texture::operator==(const Texture &other) const {
  return this->extensions == other.extensions && this->extras == other.extras &&
         this->name == other.name && this->sampler == other.sampler &&
         this->source == other.source;
}
bool Value::operator==(const Value &other) const {
  return Equals(*this, other);
}

static void swap4(unsigned int *val) {
#ifdef TINYGLTF_LITTLE_ENDIAN
  (void)val;
#else
  unsigned int tmp = *val;
  unsigned char *dst = reinterpret_cast<unsigned char *>(val);
  unsigned char *src = reinterpret_cast<unsigned char *>(&tmp);

  dst[0] = src[3];
  dst[1] = src[2];
  dst[2] = src[1];
  dst[3] = src[0];
#endif
}

static std::string JoinPath(const std::string &path0,
                            const std::string &path1) {
  if (path0.empty()) {
    return path1;
  } else {
    // check '/'
    char lastChar = *path0.rbegin();
    if (lastChar != '/') {
      return path0 + std::string("/") + path1;
    } else {
      return path0 + path1;
    }
  }
}

static std::string FindFile(const std::vector<std::string> &paths,
                            const std::string &filepath, FsCallbacks *fs) {
  if (fs == nullptr || fs->ExpandFilePath == nullptr ||
      fs->FileExists == nullptr) {
    // Error, fs callback[s] missing
    return std::string();
  }

  for (size_t i = 0; i < paths.size(); i++) {
    std::string absPath =
        fs->ExpandFilePath(JoinPath(paths[i], filepath), fs->user_data);
    if (fs->FileExists(absPath, fs->user_data)) {
      return absPath;
    }
  }

  return std::string();
}

static std::string GetFilePathExtension(const std::string &FileName) {
  if (FileName.find_last_of(".") != std::string::npos)
    return FileName.substr(FileName.find_last_of(".") + 1);
  return "";
}

static std::string GetBaseDir(const std::string &filepath) {
  if (filepath.find_last_of("/\\") != std::string::npos)
    return filepath.substr(0, filepath.find_last_of("/\\"));
  return "";
}

// https://stackoverflow.com/questions/8520560/get-a-file-name-from-a-path
static std::string GetBaseFilename(const std::string &filepath) {
  return filepath.substr(filepath.find_last_of("/\\") + 1);
}

std::string base64_encode(unsigned char const *, unsigned int len);
std::string base64_decode(std::string const &s);

/*
   base64.cpp and base64.h

   Copyright (C) 2004-2008 René Nyffenegger

   This source code is provided 'as-is', without any express or implied
   warranty. In no event will the author be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this source code must not be misrepresented; you must not
      claim that you wrote the original source code. If you use this source code
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original source code.

   3. This notice may not be removed or altered from any source distribution.

   René Nyffenegger rene.nyffenegger@adp-gmbh.ch

*/

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wconversion"
#endif
static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_encode(unsigned char const *bytes_to_encode,
                          unsigned int in_len) {
  std::string ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] =
          ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] =
          ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for (i = 0; (i < 4); i++) ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 3; j++) char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] =
        ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] =
        ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

    for (j = 0; (j < i + 1); j++) ret += base64_chars[char_array_4[j]];

    while ((i++ < 3)) ret += '=';
  }

  return ret;
}

std::string base64_decode(std::string const &encoded_string) {
  int in_len = static_cast<int>(encoded_string.size());
  int i = 0;
  int j = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];
  std::string ret;

  while (in_len-- && (encoded_string[in_] != '=') &&
         is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_];
    in_++;
    if (i == 4) {
      for (i = 0; i < 4; i++)
        char_array_4[i] =
            static_cast<unsigned char>(base64_chars.find(char_array_4[i]));

      char_array_3[0] =
          (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] =
          ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++) ret += char_array_3[i];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 4; j++) char_array_4[j] = 0;

    for (j = 0; j < 4; j++)
      char_array_4[j] =
          static_cast<unsigned char>(base64_chars.find(char_array_4[j]));

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] =
        ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
  }

  return ret;
}
#ifdef __clang__
#pragma clang diagnostic pop
#endif

static bool LoadExternalFile(std::vector<unsigned char> *out, std::string *err,
                             std::string *warn, const std::string &filename,
                             const std::string &basedir, bool required,
                             size_t reqBytes, bool checkSize, FsCallbacks *fs) {
  if (fs == nullptr || fs->FileExists == nullptr ||
      fs->ExpandFilePath == nullptr || fs->ReadWholeFile == nullptr) {
    // This is a developer error, assert() ?
    if (err) {
      (*err) += "FS callback[s] not set\n";
    }
    return false;
  }

  std::string *failMsgOut = required ? err : warn;

  out->clear();

  std::vector<std::string> paths;
  paths.push_back(basedir);
  paths.push_back(".");

  std::string filepath = FindFile(paths, filename, fs);
  if (filepath.empty() || filename.empty()) {
    if (failMsgOut) {
      (*failMsgOut) += "File not found : " + filename + "\n";
    }
    return false;
  }

  std::vector<unsigned char> buf;
  std::string fileReadErr;
  bool fileRead =
      fs->ReadWholeFile(&buf, &fileReadErr, filepath, fs->user_data);
  if (!fileRead) {
    if (failMsgOut) {
      (*failMsgOut) +=
          "File read error : " + filepath + " : " + fileReadErr + "\n";
    }
    return false;
  }

  size_t sz = buf.size();
  if (sz == 0) {
    if (failMsgOut) {
      (*failMsgOut) += "File is empty : " + filepath + "\n";
    }
    return false;
  }

  if (checkSize) {
    if (reqBytes == sz) {
      out->swap(buf);
      return true;
    } else {
      std::stringstream ss;
      ss << "File size mismatch : " << filepath << ", requestedBytes "
         << reqBytes << ", but got " << sz << std::endl;
      if (failMsgOut) {
        (*failMsgOut) += ss.str();
      }
      return false;
    }
  }

  out->swap(buf);
  return true;
}

void TinyGLTF::SetImageLoader(LoadImageDataFunction func, void *user_data) {
  LoadImageData = func;
  load_image_user_data_ = user_data;
}

#ifndef TINYGLTF_NO_STB_IMAGE
bool LoadImageData(Image *image, const int image_idx, std::string *err, std::string *warn,
                   int req_width, int req_height, const unsigned char *bytes,
                   int size, void *user_data) {
  (void)warn;

  int w, h, comp, req_comp;

  // force 32-bit textures for common Vulkan compatibility. It appears that
  // some GPU drivers do not support 24-bit images for Vulkan
  req_comp = 4;

  // if image cannot be decoded, ignore parsing and keep it by its path
  // don't break in this case
  // FIXME we should only enter this function if the image is embedded. If
  // image->uri references
  // an image file, it should be left as it is. Image loading should not be
  // mandatory (to support other formats)
  unsigned char *data =
      stbi_load_from_memory(bytes, size, &w, &h, &comp, req_comp);
  if (!data) {
    // NOTE: you can use `warn` instead of `err`
    if (err) {
      (*err) += "Unknown image format. STB cannot decode image data for image[" + std::to_string(image_idx) + "] name = \"" + image->name + "\". Proably 16bit PNG?\n";
    }
    return false;
  }

  if (w < 1 || h < 1) {
    free(data);
    if (err) {
      (*err) += "Invalid image data for image[" + std::to_string(image_idx) + "] name = \"" + image->name + "\"\n";
    }
    return false;
  }

  if (req_width > 0) {
    if (req_width != w) {
      free(data);
      if (err) {
        (*err) += "Image width mismatch for image[" + std::to_string(image_idx) + "] name = \"" + image->name + "\"\n";
      }
      return false;
    }
  }

  if (req_height > 0) {
    if (req_height != h) {
      free(data);
      if (err) {
        (*err) += "Image height mismatch. for image[" + std::to_string(image_idx) + "] name = \"" + image->name + "\"\n";
      }
      return false;
    }
  }

  image->width = w;
  image->height = h;
  image->component = req_comp;
  image->image.resize(static_cast<size_t>(w * h * req_comp));
  std::copy(data, data + w * h * req_comp, image->image.begin());

  free(data);

  return true;
}
#endif

void TinyGLTF::SetImageWriter(WriteImageDataFunction func, void *user_data) {
  WriteImageData = func;
  write_image_user_data_ = user_data;
}

#ifndef TINYGLTF_NO_STB_IMAGE_WRITE
static void WriteToMemory_stbi(void *context, void *data, int size) {
  std::vector<unsigned char> *buffer =
      reinterpret_cast<std::vector<unsigned char> *>(context);

  unsigned char *pData = reinterpret_cast<unsigned char *>(data);

  buffer->insert(buffer->end(), pData, pData + size);
}

bool WriteImageData(const std::string *basepath, const std::string *filename,
                    Image *image, bool embedImages, void *fsPtr) {
  const std::string ext = GetFilePathExtension(*filename);

  // Write image to temporary buffer
  std::string header;
  std::vector<unsigned char> data;

  if (ext == "png") {
    if (!stbi_write_png_to_func(WriteToMemory_stbi, &data, image->width,
                                image->height, image->component,
                                &image->image[0], 0)) {
      return false;
    }
    header = "data:image/png;base64,";
  } else if (ext == "jpg") {
    if (!stbi_write_jpg_to_func(WriteToMemory_stbi, &data, image->width,
                                image->height, image->component,
                                &image->image[0], 100)) {
      return false;
    }
    header = "data:image/jpeg;base64,";
  } else if (ext == "bmp") {
    if (!stbi_write_bmp_to_func(WriteToMemory_stbi, &data, image->width,
                                image->height, image->component,
                                &image->image[0])) {
      return false;
    }
    header = "data:image/bmp;base64,";
  } else if (!embedImages) {
    // Error: can't output requested format to file
    return false;
  }

  if (embedImages) {
    // Embed base64-encoded image into URI
    if (data.size()) {
      image->uri =
          header +
          base64_encode(&data[0], static_cast<unsigned int>(data.size()));
    } else {
      // Throw error?
    }
  } else {
    // Write image to disc
    FsCallbacks *fs = reinterpret_cast<FsCallbacks *>(fsPtr);
    if ((fs != nullptr) && (fs->WriteWholeFile != nullptr)) {
      const std::string imagefilepath = JoinPath(*basepath, *filename);
      std::string writeError;
      if (!fs->WriteWholeFile(&writeError, imagefilepath, data,
                              fs->user_data)) {
        // Could not write image file to disc; Throw error ?
        return false;
      }
    } else {
      // Throw error?
    }
    image->uri = *filename;
  }

  return true;
}
#endif

void TinyGLTF::SetFsCallbacks(FsCallbacks callbacks) { fs = callbacks; }

#ifndef TINYGLTF_NO_FS
// Default implementations of filesystem functions

bool FileExists(const std::string &abs_filename, void *) {
  bool ret;
#ifdef TINYGLTF_ANDROID_LOAD_FROM_ASSETS
    if (asset_manager) {
        AAsset* asset = AAssetManager_open(asset_manager, abs_filename.c_str(), AASSET_MODE_STREAMING);
        if (!asset) {
            return false;
        }
        AAsset_close(asset);
        ret = true;
    } else {
      return false;
    }
#else
#ifdef _WIN32
  FILE *fp;
  errno_t err = fopen_s(&fp, abs_filename.c_str(), "rb");
  if (err != 0) {
    return false;
  }
#else
  FILE *fp = fopen(abs_filename.c_str(), "rb");
#endif
  if (fp) {
    ret = true;
    fclose(fp);
  } else {
    ret = false;
  }
#endif

  return ret;
}

std::string ExpandFilePath(const std::string &filepath, void *) {
#ifdef _WIN32
  DWORD len = ExpandEnvironmentStringsA(filepath.c_str(), NULL, 0);
  char *str = new char[len];
  ExpandEnvironmentStringsA(filepath.c_str(), str, len);

  std::string s(str);

  delete[] str;

  return s;
#else

#if defined(TARGET_OS_IPHONE) || defined(TARGET_IPHONE_SIMULATOR) || \
    defined(__ANDROID__) || defined(__EMSCRIPTEN__)
  // no expansion
  std::string s = filepath;
#else
  std::string s;
  wordexp_t p;

  if (filepath.empty()) {
    return "";
  }

  // char** w;
  int ret = wordexp(filepath.c_str(), &p, 0);
  if (ret) {
    // err
    s = filepath;
    return s;
  }

  // Use first element only.
  if (p.we_wordv) {
    s = std::string(p.we_wordv[0]);
    wordfree(&p);
  } else {
    s = filepath;
  }

#endif

  return s;
#endif
}

bool ReadWholeFile(std::vector<unsigned char> *out, std::string *err,
                   const std::string &filepath, void *) {
#ifdef TINYGLTF_ANDROID_LOAD_FROM_ASSETS
  if (asset_manager) {
    AAsset* asset = AAssetManager_open(asset_manager, filepath.c_str(), AASSET_MODE_STREAMING);
    if (!asset) {
      if (err) {
        (*err) += "File open error : " + filepath + "\n";
      }
      return false;
    }
    size_t size = AAsset_getLength(asset);
    if (size <= 0) {
      if (err) {
        (*err) += "Invalid file size : " + filepath +
                  " (does the path point to a directory?)";
      }
    }
    out->resize(size);
    AAsset_read(asset, reinterpret_cast<char *>(&out->at(0)), size);
    AAsset_close(asset);
    return true;
  } else {
    if (err) {
      (*err) += "No asset manager specified : " + filepath + "\n";
    }
    return false;
  }
#else
  std::ifstream f(filepath.c_str(), std::ifstream::binary);
  if (!f) {
    if (err) {
      (*err) += "File open error : " + filepath + "\n";
    }
    return false;
  }

  f.seekg(0, f.end);
  size_t sz = static_cast<size_t>(f.tellg());
  f.seekg(0, f.beg);

  if (int(sz) < 0) {
    if (err) {
      (*err) += "Invalid file size : " + filepath +
                " (does the path point to a directory?)";
    }
    return false;
  } else if (sz == 0) {
    if (err) {
      (*err) += "File is empty : " + filepath + "\n";
    }
    return false;
  }

  out->resize(sz);
  f.read(reinterpret_cast<char *>(&out->at(0)),
         static_cast<std::streamsize>(sz));
  f.close();

  return true;
#endif
}

bool WriteWholeFile(std::string *err, const std::string &filepath,
                    const std::vector<unsigned char> &contents, void *) {
  std::ofstream f(filepath.c_str(), std::ofstream::binary);
  if (!f) {
    if (err) {
      (*err) += "File open error for writing : " + filepath + "\n";
    }
    return false;
  }

  f.write(reinterpret_cast<const char *>(&contents.at(0)),
          static_cast<std::streamsize>(contents.size()));
  if (!f) {
    if (err) {
      (*err) += "File write error: " + filepath + "\n";
    }
    return false;
  }

  f.close();
  return true;
}

#endif  // TINYGLTF_NO_FS

static std::string MimeToExt(const std::string &mimeType) {
  if (mimeType == "image/jpeg") {
    return "jpg";
  } else if (mimeType == "image/png") {
    return "png";
  } else if (mimeType == "image/bmp") {
    return "bmp";
  } else if (mimeType == "image/gif") {
    return "gif";
  }

  return "";
}

static void UpdateImageObject(Image &image, std::string &baseDir, int index,
                              bool embedImages,
                              WriteImageDataFunction *WriteImageData = nullptr,
                              void *user_data = nullptr) {
  std::string filename;
  std::string ext;

  // If image have uri. Use it it as a filename
  if (image.uri.size()) {
    filename = GetBaseFilename(image.uri);
    ext = GetFilePathExtension(filename);

  } else if (image.name.size()) {
    ext = MimeToExt(image.mimeType);
    // Otherwise use name as filename
    filename = image.name + "." + ext;
  } else {
    ext = MimeToExt(image.mimeType);
    // Fallback to index of image as filename
    filename = std::to_string(index) + "." + ext;
  }

  // If callback is set, modify image data object
  if (*WriteImageData != nullptr) {
    std::string uri;
    (*WriteImageData)(&baseDir, &filename, &image, embedImages, user_data);
  }
}

bool IsDataURI(const std::string &in) {
  std::string header = "data:application/octet-stream;base64,";
  if (in.find(header) == 0) {
    return true;
  }

  header = "data:image/jpeg;base64,";
  if (in.find(header) == 0) {
    return true;
  }

  header = "data:image/png;base64,";
  if (in.find(header) == 0) {
    return true;
  }

  header = "data:image/bmp;base64,";
  if (in.find(header) == 0) {
    return true;
  }

  header = "data:image/gif;base64,";
  if (in.find(header) == 0) {
    return true;
  }

  header = "data:text/plain;base64,";
  if (in.find(header) == 0) {
    return true;
  }

  header = "data:application/gltf-buffer;base64,";
  if (in.find(header) == 0) {
    return true;
  }

  return false;
}

bool DecodeDataURI(std::vector<unsigned char> *out, std::string &mime_type,
                   const std::string &in, size_t reqBytes, bool checkSize) {
  std::string header = "data:application/octet-stream;base64,";
  std::string data;
  if (in.find(header) == 0) {
    data = base64_decode(in.substr(header.size()));  // cut mime string.
  }

  if (data.empty()) {
    header = "data:image/jpeg;base64,";
    if (in.find(header) == 0) {
      mime_type = "image/jpeg";
      data = base64_decode(in.substr(header.size()));  // cut mime string.
    }
  }

  if (data.empty()) {
    header = "data:image/png;base64,";
    if (in.find(header) == 0) {
      mime_type = "image/png";
      data = base64_decode(in.substr(header.size()));  // cut mime string.
    }
  }

  if (data.empty()) {
    header = "data:image/bmp;base64,";
    if (in.find(header) == 0) {
      mime_type = "image/bmp";
      data = base64_decode(in.substr(header.size()));  // cut mime string.
    }
  }

  if (data.empty()) {
    header = "data:image/gif;base64,";
    if (in.find(header) == 0) {
      mime_type = "image/gif";
      data = base64_decode(in.substr(header.size()));  // cut mime string.
    }
  }

  if (data.empty()) {
    header = "data:text/plain;base64,";
    if (in.find(header) == 0) {
      mime_type = "text/plain";
      data = base64_decode(in.substr(header.size()));
    }
  }

  if (data.empty()) {
    header = "data:application/gltf-buffer;base64,";
    if (in.find(header) == 0) {
      data = base64_decode(in.substr(header.size()));
    }
  }

  if (data.empty()) {
    return false;
  }

  if (checkSize) {
    if (data.size() != reqBytes) {
      return false;
    }
    out->resize(reqBytes);
  } else {
    out->resize(data.size());
  }
  std::copy(data.begin(), data.end(), out->begin());
  return true;
}

static bool ParseJsonAsValue(Value *ret, const json &o) {
  Value val{};
  switch (o.type()) {
    case json::value_t::object: {
      Value::Object value_object;
      for (auto it = o.begin(); it != o.end(); it++) {
        Value entry;
        ParseJsonAsValue(&entry, it.value());
        if (entry.Type() != NULL_TYPE) value_object[it.key()] = entry;
      }
      if (value_object.size() > 0) val = Value(value_object);
    } break;
    case json::value_t::array: {
      Value::Array value_array;
      for (auto it = o.begin(); it != o.end(); it++) {
        Value entry;
        ParseJsonAsValue(&entry, it.value());
        if (entry.Type() != NULL_TYPE) value_array.push_back(entry);
      }
      if (value_array.size() > 0) val = Value(value_array);
    } break;
    case json::value_t::string:
      val = Value(o.get<std::string>());
      break;
    case json::value_t::boolean:
      val = Value(o.get<bool>());
      break;
    case json::value_t::number_integer:
    case json::value_t::number_unsigned:
      val = Value(static_cast<int>(o.get<int64_t>()));
      break;
    case json::value_t::number_float:
      val = Value(o.get<double>());
      break;
    case json::value_t::null:
    case json::value_t::discarded:
      // default:
      break;
  }
  if (ret) *ret = val;

  return val.Type() != NULL_TYPE;
}

static bool ParseExtrasProperty(Value *ret, const json &o) {
  json::const_iterator it = o.find("extras");
  if (it == o.end()) {
    return false;
  }

  return ParseJsonAsValue(ret, it.value());
}

static bool ParseBooleanProperty(bool *ret, std::string *err, const json &o,
                                 const std::string &property,
                                 const bool required,
                                 const std::string &parent_node = "") {
  json::const_iterator it = o.find(property);
  if (it == o.end()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is missing";
        if (!parent_node.empty()) {
          (*err) += " in " + parent_node;
        }
        (*err) += ".\n";
      }
    }
    return false;
  }

  if (!it.value().is_boolean()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is not a bool type.\n";
      }
    }
    return false;
  }

  if (ret) {
    (*ret) = it.value().get<bool>();
  }

  return true;
}

static bool ParseNumberProperty(double *ret, std::string *err, const json &o,
                                const std::string &property,
                                const bool required,
                                const std::string &parent_node = "") {
  json::const_iterator it = o.find(property);
  if (it == o.end()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is missing";
        if (!parent_node.empty()) {
          (*err) += " in " + parent_node;
        }
        (*err) += ".\n";
      }
    }
    return false;
  }

  if (!it.value().is_number()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is not a number type.\n";
      }
    }
    return false;
  }

  if (ret) {
    (*ret) = it.value().get<double>();
  }

  return true;
}

static bool ParseNumberArrayProperty(std::vector<double> *ret, std::string *err,
                                     const json &o, const std::string &property,
                                     bool required,
                                     const std::string &parent_node = "") {
  json::const_iterator it = o.find(property);
  if (it == o.end()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is missing";
        if (!parent_node.empty()) {
          (*err) += " in " + parent_node;
        }
        (*err) += ".\n";
      }
    }
    return false;
  }

  if (!it.value().is_array()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is not an array";
        if (!parent_node.empty()) {
          (*err) += " in " + parent_node;
        }
        (*err) += ".\n";
      }
    }
    return false;
  }

  ret->clear();
  for (json::const_iterator i = it.value().begin(); i != it.value().end();
       i++) {
    if (!i.value().is_number()) {
      if (required) {
        if (err) {
          (*err) += "'" + property + "' property is not a number.\n";
          if (!parent_node.empty()) {
            (*err) += " in " + parent_node;
          }
          (*err) += ".\n";
        }
      }
      return false;
    }
    ret->push_back(i.value());
  }

  return true;
}

static bool ParseStringProperty(
    std::string *ret, std::string *err, const json &o,
    const std::string &property, bool required,
    const std::string &parent_node = std::string()) {
  json::const_iterator it = o.find(property);
  if (it == o.end()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is missing";
        if (parent_node.empty()) {
          (*err) += ".\n";
        } else {
          (*err) += " in `" + parent_node + "'.\n";
        }
      }
    }
    return false;
  }

  if (!it.value().is_string()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is not a string type.\n";
      }
    }
    return false;
  }

  if (ret) {
    (*ret) = it.value().get<std::string>();
  }

  return true;
}

static bool ParseStringIntProperty(std::map<std::string, int> *ret,
                                   std::string *err, const json &o,
                                   const std::string &property, bool required,
                                   const std::string &parent = "") {
  json::const_iterator it = o.find(property);
  if (it == o.end()) {
    if (required) {
      if (err) {
        if (!parent.empty()) {
          (*err) +=
              "'" + property + "' property is missing in " + parent + ".\n";
        } else {
          (*err) += "'" + property + "' property is missing.\n";
        }
      }
    }
    return false;
  }

  // Make sure we are dealing with an object / dictionary.
  if (!it.value().is_object()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is not an object.\n";
      }
    }
    return false;
  }

  ret->clear();
  const json &dict = it.value();

  json::const_iterator dictIt(dict.begin());
  json::const_iterator dictItEnd(dict.end());

  for (; dictIt != dictItEnd; ++dictIt) {
    if (!dictIt.value().is_number()) {
      if (required) {
        if (err) {
          (*err) += "'" + property + "' value is not an int.\n";
        }
      }
      return false;
    }

    // Insert into the list.
    (*ret)[dictIt.key()] = static_cast<int>(dictIt.value());
  }
  return true;
}

static bool ParseJSONProperty(std::map<std::string, double> *ret,
                              std::string *err, const json &o,
                              const std::string &property, bool required) {
  json::const_iterator it = o.find(property);
  if (it == o.end()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is missing. \n'";
      }
    }
    return false;
  }

  if (!it.value().is_object()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is not a JSON object.\n";
      }
    }
    return false;
  }

  ret->clear();
  const json &obj = it.value();
  json::const_iterator it2(obj.begin());
  json::const_iterator itEnd(obj.end());
  for (; it2 != itEnd; it2++) {
    if (it2.value().is_number())
      ret->insert(std::pair<std::string, double>(it2.key(), it2.value()));
  }

  return true;
}

static bool ParseParameterProperty(Parameter *param, std::string *err,
                                   const json &o, const std::string &prop,
                                   bool required) {
  // A parameter value can either be a string or an array of either a boolean or
  // a number. Booleans of any kind aren't supported here. Granted, it
  // complicates the Parameter structure and breaks it semantically in the sense
  // that the client probably works off the assumption that if the string is
  // empty the vector is used, etc. Would a tagged union work?
  if (ParseStringProperty(&param->string_value, err, o, prop, false)) {
    // Found string property.
    return true;
  } else if (ParseNumberArrayProperty(&param->number_array, err, o, prop,
                                      false)) {
    // Found a number array.
    return true;
  } else if (ParseNumberProperty(&param->number_value, err, o, prop, false)) {
    return param->has_number_value = true;
  } else if (ParseJSONProperty(&param->json_double_value, err, o, prop,
                               false)) {
    return true;
  } else if (ParseBooleanProperty(&param->bool_value, err, o, prop, false)) {
    return true;
  } else {
    if (required) {
      if (err) {
        (*err) += "parameter must be a string or number / number array.\n";
      }
    }
    return false;
  }
}

static bool ParseExtensionsProperty(ExtensionMap *ret, std::string *err,
                                    const json &o) {
  (void)err;

  json::const_iterator it = o.find("extensions");
  if (it == o.end()) {
    return false;
  }
  if (!it.value().is_object()) {
    return false;
  }
  ExtensionMap extensions;
  json::const_iterator extIt = it.value().begin();
  for (; extIt != it.value().end(); extIt++) {
    if (!extIt.value().is_object()) continue;
	if (!ParseJsonAsValue(&extensions[extIt.key()], extIt.value())) {
      if (!extIt.key().empty()) {
        // create empty object so that an extension object is still of type object
        extensions[extIt.key()] = Value{ Value::Object{} };
      }
	}
  }
  if (ret) {
    (*ret) = extensions;
  }
  return true;
}

static bool ParseAsset(Asset *asset, std::string *err, const json &o) {
  ParseStringProperty(&asset->version, err, o, "version", true, "Asset");
  ParseStringProperty(&asset->generator, err, o, "generator", false, "Asset");
  ParseStringProperty(&asset->minVersion, err, o, "minVersion", false, "Asset");

  ParseExtensionsProperty(&asset->extensions, err, o);

  // Unity exporter version is added as extra here
  ParseExtrasProperty(&(asset->extras), o);

  return true;
}

static bool ParseImage(Image *image, const int image_idx, std::string *err, std::string *warn,
                       const json &o, const std::string &basedir,
                       FsCallbacks *fs,
                       LoadImageDataFunction *LoadImageData = nullptr,
                       void *load_image_user_data = nullptr) {
  // A glTF image must either reference a bufferView or an image uri

  // schema says oneOf [`bufferView`, `uri`]
  // TODO(syoyo): Check the type of each parameters.
  bool hasBufferView = (o.find("bufferView") != o.end());
  bool hasURI = (o.find("uri") != o.end());

  ParseStringProperty(&image->name, err, o, "name", false);

  if (hasBufferView && hasURI) {
    // Should not both defined.
    if (err) {
      (*err) +=
          "Only one of `bufferView` or `uri` should be defined, but both are "
          "defined for image[" + std::to_string(image_idx) + "] name = \"" + image->name + "\"\n";
    }
    return false;
  }

  if (!hasBufferView && !hasURI) {
    if (err) {
      (*err) += "Neither required `bufferView` nor `uri` defined for image[" + std::to_string(image_idx) + "] name = \"" + image->name + "\"\n";
    }
    return false;
  }

  ParseExtensionsProperty(&image->extensions, err, o);
  ParseExtrasProperty(&image->extras, o);

  if (hasBufferView) {
    double bufferView = -1;
    if (!ParseNumberProperty(&bufferView, err, o, "bufferView", true)) {
      if (err) {
        (*err) += "Failed to parse `bufferView` for image[" + std::to_string(image_idx) + "] name = \"" + image->name + "\"\n";
      }
      return false;
    }

    std::string mime_type;
    ParseStringProperty(&mime_type, err, o, "mimeType", false);

    double width = 0.0;
    ParseNumberProperty(&width, err, o, "width", false);

    double height = 0.0;
    ParseNumberProperty(&height, err, o, "height", false);

    // Just only save some information here. Loading actual image data from
    // bufferView is done after this `ParseImage` function.
    image->bufferView = static_cast<int>(bufferView);
    image->mimeType = mime_type;
    image->width = static_cast<int>(width);
    image->height = static_cast<int>(height);

    return true;
  }

  // Parse URI & Load image data.

  std::string uri;
  std::string tmp_err;
  if (!ParseStringProperty(&uri, &tmp_err, o, "uri", true)) {
    if (err) {
      (*err) += "Failed to parse `uri` for image[" + std::to_string(image_idx) + "] name = \"" + image->name + "\".\n";
    }
    return false;
  }

  std::vector<unsigned char> img;

  if (IsDataURI(uri)) {
    if (!DecodeDataURI(&img, image->mimeType, uri, 0, false)) {
      if (err) {
        (*err) += "Failed to decode 'uri' for image[" + std::to_string(image_idx) + "] name = [" + image->name + "]\n";
      }
      return false;
    }
  } else {
    // Assume external file
    // Keep texture path (for textures that cannot be decoded)
    image->uri = uri;
#ifdef TINYGLTF_NO_EXTERNAL_IMAGE
    return true;
#endif
    if (!LoadExternalFile(&img, err, warn, uri, basedir, false, 0, false, fs)) {
      if (warn) {
        (*warn) += "Failed to load external 'uri' for image[" + std::to_string(image_idx) + "] name = [" + image->name + "]\n";
      }
      // If the image cannot be loaded, keep uri as image->uri.
      return true;
    }

    if (img.empty()) {
      if (warn) {
        (*warn) += "Image data is empty for image[" + std::to_string(image_idx) + "] name = [" + image->name + "] \n";
      }
      return false;
    }
  }

  if (*LoadImageData == nullptr) {
    if (err) {
      (*err) += "No LoadImageData callback specified.\n";
    }
    return false;
  }
  return (*LoadImageData)(image, image_idx, err, warn, 0, 0, &img.at(0),
                          static_cast<int>(img.size()), load_image_user_data);
}

static bool ParseTexture(Texture *texture, std::string *err, const json &o,
                         const std::string &basedir) {
  (void)basedir;
  double sampler = -1.0;
  double source = -1.0;
  ParseNumberProperty(&sampler, err, o, "sampler", false);

  ParseNumberProperty(&source, err, o, "source", false);

  texture->sampler = static_cast<int>(sampler);
  texture->source = static_cast<int>(source);

  ParseExtensionsProperty(&texture->extensions, err, o);
  ParseExtrasProperty(&texture->extras, o);

  ParseStringProperty(&texture->name, err, o, "name", false);

  return true;
}

static bool ParseBuffer(Buffer *buffer, std::string *err, const json &o,
                        FsCallbacks *fs, const std::string &basedir,
                        bool is_binary = false,
                        const unsigned char *bin_data = nullptr,
                        size_t bin_size = 0) {
  double byteLength;
  if (!ParseNumberProperty(&byteLength, err, o, "byteLength", true, "Buffer")) {
    return false;
  }

  // In glTF 2.0, uri is not mandatory anymore
  buffer->uri.clear();
  ParseStringProperty(&buffer->uri, err, o, "uri", false, "Buffer");

  // having an empty uri for a non embedded image should not be valid
  if (!is_binary && buffer->uri.empty()) {
    if (err) {
      (*err) += "'uri' is missing from non binary glTF file buffer.\n";
    }
  }

  json::const_iterator type = o.find("type");
  if (type != o.end()) {
    if (type.value().is_string()) {
      const std::string &ty = type.value();
      if (ty.compare("arraybuffer") == 0) {
        // buffer.type = "arraybuffer";
      }
    }
  }

  size_t bytes = static_cast<size_t>(byteLength);
  if (is_binary) {
    // Still binary glTF accepts external dataURI.
    if (!buffer->uri.empty()) {
      // First try embedded data URI.
      if (IsDataURI(buffer->uri)) {
        std::string mime_type;
        if (!DecodeDataURI(&buffer->data, mime_type, buffer->uri, bytes,
                           true)) {
          if (err) {
            (*err) +=
                "Failed to decode 'uri' : " + buffer->uri + " in Buffer\n";
          }
          return false;
        }
      } else {
        // External .bin file.
        if (!LoadExternalFile(&buffer->data, err, /* warn */ nullptr,
                              buffer->uri, basedir, true, bytes, true, fs)) {
          return false;
        }
      }
    } else {
      // load data from (embedded) binary data

      if ((bin_size == 0) || (bin_data == nullptr)) {
        if (err) {
          (*err) += "Invalid binary data in `Buffer'.\n";
        }
        return false;
      }

      if (byteLength > bin_size) {
        if (err) {
          std::stringstream ss;
          ss << "Invalid `byteLength'. Must be equal or less than binary size: "
                "`byteLength' = "
             << byteLength << ", binary size = " << bin_size << std::endl;
          (*err) += ss.str();
        }
        return false;
      }

      // Read buffer data
      buffer->data.resize(static_cast<size_t>(byteLength));
      memcpy(&(buffer->data.at(0)), bin_data, static_cast<size_t>(byteLength));
    }

  } else {
    if (IsDataURI(buffer->uri)) {
      std::string mime_type;
      if (!DecodeDataURI(&buffer->data, mime_type, buffer->uri, bytes, true)) {
        if (err) {
          (*err) += "Failed to decode 'uri' : " + buffer->uri + " in Buffer\n";
        }
        return false;
      }
    } else {
      // Assume external .bin file.
      if (!LoadExternalFile(&buffer->data, err, /* warn */ nullptr, buffer->uri,
                            basedir, true, bytes, true, fs)) {
        return false;
      }
    }
  }

  ParseStringProperty(&buffer->name, err, o, "name", false);

  return true;
}

static bool ParseBufferView(BufferView *bufferView, std::string *err,
                            const json &o) {
  double buffer = -1.0;
  if (!ParseNumberProperty(&buffer, err, o, "buffer", true, "BufferView")) {
    return false;
  }

  double byteOffset = 0.0;
  ParseNumberProperty(&byteOffset, err, o, "byteOffset", false);

  double byteLength = 1.0;
  if (!ParseNumberProperty(&byteLength, err, o, "byteLength", true,
                           "BufferView")) {
    return false;
  }

  size_t byteStride = 0;
  double byteStrideValue = 0.0;
  if (!ParseNumberProperty(&byteStrideValue, err, o, "byteStride", false)) {
    // Spec says: When byteStride of referenced bufferView is not defined, it
    // means that accessor elements are tightly packed, i.e., effective stride
    // equals the size of the element.
    // We cannot determine the actual byteStride until Accessor are parsed, thus
    // set 0(= tightly packed) here(as done in OpenGL's VertexAttribPoiner)
    byteStride = 0;
  } else {
    byteStride = static_cast<size_t>(byteStrideValue);
  }

  if ((byteStride > 252) || ((byteStride % 4) != 0)) {
    if (err) {
      std::stringstream ss;
      ss << "Invalid `byteStride' value. `byteStride' must be the multiple of "
            "4 : "
         << byteStride << std::endl;

      (*err) += ss.str();
    }
    return false;
  }

  double target = 0.0;
  ParseNumberProperty(&target, err, o, "target", false);
  int targetValue = static_cast<int>(target);
  if ((targetValue == TINYGLTF_TARGET_ARRAY_BUFFER) ||
      (targetValue == TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER)) {
    // OK
  } else {
    targetValue = 0;
  }
  bufferView->target = targetValue;

  ParseStringProperty(&bufferView->name, err, o, "name", false);

  bufferView->buffer = static_cast<int>(buffer);
  bufferView->byteOffset = static_cast<size_t>(byteOffset);
  bufferView->byteLength = static_cast<size_t>(byteLength);
  bufferView->byteStride = static_cast<size_t>(byteStride);

  return true;
}

static bool ParseAccessor(Accessor *accessor, std::string *err, const json &o) {
  double bufferView = -1.0;
  ParseNumberProperty(&bufferView, err, o, "bufferView", false, "Accessor");

  double byteOffset = 0.0;
  ParseNumberProperty(&byteOffset, err, o, "byteOffset", false, "Accessor");

  bool normalized = false;
  ParseBooleanProperty(&normalized, err, o, "normalized", false, "Accessor");

  double componentType = 0.0;
  if (!ParseNumberProperty(&componentType, err, o, "componentType", true,
                           "Accessor")) {
    return false;
  }

  double count = 0.0;
  if (!ParseNumberProperty(&count, err, o, "count", true, "Accessor")) {
    return false;
  }

  std::string type;
  if (!ParseStringProperty(&type, err, o, "type", true, "Accessor")) {
    return false;
  }

  if (type.compare("SCALAR") == 0) {
    accessor->type = TINYGLTF_TYPE_SCALAR;
  } else if (type.compare("VEC2") == 0) {
    accessor->type = TINYGLTF_TYPE_VEC2;
  } else if (type.compare("VEC3") == 0) {
    accessor->type = TINYGLTF_TYPE_VEC3;
  } else if (type.compare("VEC4") == 0) {
    accessor->type = TINYGLTF_TYPE_VEC4;
  } else if (type.compare("MAT2") == 0) {
    accessor->type = TINYGLTF_TYPE_MAT2;
  } else if (type.compare("MAT3") == 0) {
    accessor->type = TINYGLTF_TYPE_MAT3;
  } else if (type.compare("MAT4") == 0) {
    accessor->type = TINYGLTF_TYPE_MAT4;
  } else {
    std::stringstream ss;
    ss << "Unsupported `type` for accessor object. Got \"" << type << "\"\n";
    if (err) {
      (*err) += ss.str();
    }
    return false;
  }

  ParseStringProperty(&accessor->name, err, o, "name", false);

  accessor->minValues.clear();
  accessor->maxValues.clear();
  ParseNumberArrayProperty(&accessor->minValues, err, o, "min", false,
                           "Accessor");

  ParseNumberArrayProperty(&accessor->maxValues, err, o, "max", false,
                           "Accessor");

  accessor->count = static_cast<size_t>(count);
  accessor->bufferView = static_cast<int>(bufferView);
  accessor->byteOffset = static_cast<size_t>(byteOffset);
  accessor->normalized = normalized;
  {
    int comp = static_cast<int>(componentType);
    if (comp >= TINYGLTF_COMPONENT_TYPE_BYTE &&
        comp <= TINYGLTF_COMPONENT_TYPE_DOUBLE) {
      // OK
      accessor->componentType = comp;
    } else {
      std::stringstream ss;
      ss << "Invalid `componentType` in accessor. Got " << comp << "\n";
      if (err) {
        (*err) += ss.str();
      }
      return false;
    }
  }

  ParseExtrasProperty(&(accessor->extras), o);

  return true;
}

#ifdef TINYGLTF_ENABLE_DRACO

static void DecodeIndexBuffer(draco::Mesh* mesh, size_t componentSize, std::vector<uint8_t>& outBuffer)
{
  if (componentSize == 4)
  {
    assert(sizeof(mesh->face(draco::FaceIndex(0))[0]) == componentSize);
    memcpy(outBuffer.data(), &mesh->face(draco::FaceIndex(0))[0], outBuffer.size());
  }
  else
  {
    size_t faceStride = componentSize * 3;
    for (draco::FaceIndex f(0); f < mesh->num_faces(); ++f)
    {
      const draco::Mesh::Face& face = mesh->face(f);
      if (componentSize == 2)
      {
        uint16_t indices[3] = { (uint16_t)face[0].value(), (uint16_t)face[1].value(), (uint16_t)face[2].value() };
        memcpy(outBuffer.data() + f.value() * faceStride, &indices[0], faceStride);
      }
      else
      {
        uint8_t indices[3] = { (uint8_t)face[0].value(), (uint8_t)face[1].value(), (uint8_t)face[2].value() };
        memcpy(outBuffer.data() + f.value() * faceStride, &indices[0], faceStride);
      }
    }
  }
}

template<typename T>
static bool GetAttributeForAllPoints(draco::Mesh* mesh, const draco::PointAttribute* pAttribute, std::vector<uint8_t>& outBuffer)
{
  size_t byteOffset = 0;
  T values[4] = { 0, 0, 0, 0 };
  for (draco::PointIndex i(0); i < mesh->num_points(); ++i)
  {
    const draco::AttributeValueIndex val_index = pAttribute->mapped_index(i);
    if (!pAttribute->ConvertValue<T>(val_index, pAttribute->num_components(), values))
      return false;

    memcpy(outBuffer.data() + byteOffset, &values[0], sizeof(T) * pAttribute->num_components());
    byteOffset += sizeof(T) * pAttribute->num_components();
  }

  return true;
}

static bool GetAttributeForAllPoints(uint32_t componentType, draco::Mesh* mesh, const draco::PointAttribute* pAttribute, std::vector<uint8_t>& outBuffer)
{
  bool decodeResult = false;
  switch (componentType)
  {
  case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
    decodeResult = GetAttributeForAllPoints<uint8_t>(mesh, pAttribute, outBuffer);
    break;
  case TINYGLTF_COMPONENT_TYPE_BYTE:
    decodeResult = GetAttributeForAllPoints<int8_t>(mesh, pAttribute, outBuffer);
    break;
  case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
    decodeResult = GetAttributeForAllPoints<uint16_t>(mesh, pAttribute, outBuffer);
    break;
  case TINYGLTF_COMPONENT_TYPE_SHORT:
    decodeResult = GetAttributeForAllPoints<int16_t>(mesh, pAttribute, outBuffer);
    break;
  case TINYGLTF_COMPONENT_TYPE_INT:
    decodeResult = GetAttributeForAllPoints<int32_t>(mesh, pAttribute, outBuffer);
    break;
  case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
    decodeResult = GetAttributeForAllPoints<uint32_t>(mesh, pAttribute, outBuffer);
    break;
  case TINYGLTF_COMPONENT_TYPE_FLOAT:
    decodeResult = GetAttributeForAllPoints<float>(mesh, pAttribute, outBuffer);
    break;
  case TINYGLTF_COMPONENT_TYPE_DOUBLE:
    decodeResult = GetAttributeForAllPoints<double>(mesh, pAttribute, outBuffer);
    break;
  default:
    return false;
  }

  return decodeResult;
}

static bool ParseDracoExtension(Primitive *primitive, Model *model, std::string *err, const Value &dracoExtensionValue)
{
  auto bufferViewValue = dracoExtensionValue.Get("bufferView");
  if (!bufferViewValue.IsInt())
    return false;
  auto attributesValue = dracoExtensionValue.Get("attributes");
  if (!attributesValue.IsObject())
    return false;

  auto attributesObject = attributesValue.Get<Value::Object>();
  int bufferView = bufferViewValue.Get<int>();

  BufferView& view = model->bufferViews[bufferView];
  Buffer& buffer = model->buffers[view.buffer];
  // BufferView has already been decoded
  if (view.dracoDecoded)
    return true;
  view.dracoDecoded = true;

  const char* bufferViewData = reinterpret_cast<const char*>(buffer.data.data() + view.byteOffset);
  size_t bufferViewSize = view.byteLength;

  // decode draco
  draco::DecoderBuffer decoderBuffer;
  decoderBuffer.Init(bufferViewData, bufferViewSize);
  draco::Decoder decoder;
  auto decodeResult = decoder.DecodeMeshFromBuffer(&decoderBuffer);
  if (!decodeResult.ok()) {
    return false;
  }
  const std::unique_ptr<draco::Mesh>& mesh = decodeResult.value();

  // create new bufferView for indices
  if (primitive->indices >= 0)
  {
    int32_t componentSize = GetComponentSizeInBytes(model->accessors[primitive->indices].componentType);
    Buffer decodedIndexBuffer;
    decodedIndexBuffer.data.resize(mesh->num_faces() * 3 * componentSize);

    DecodeIndexBuffer(mesh.get(), componentSize, decodedIndexBuffer.data);

    model->buffers.emplace_back(std::move(decodedIndexBuffer));

    BufferView decodedIndexBufferView;
    decodedIndexBufferView.buffer = int(model->buffers.size() - 1);
    decodedIndexBufferView.byteLength = int(mesh->num_faces() * 3 * componentSize);
    decodedIndexBufferView.byteOffset = 0;
    decodedIndexBufferView.byteStride = 0;
    decodedIndexBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
    model->bufferViews.emplace_back(std::move(decodedIndexBufferView));

    model->accessors[primitive->indices].bufferView = int(model->bufferViews.size() - 1);
    model->accessors[primitive->indices].count = int(mesh->num_faces() * 3);
  }

  for (const auto& attribute : attributesObject)
  {
    if (!attribute.second.IsInt())
      return false;
    auto primitiveAttribute = primitive->attributes.find(attribute.first);
    if (primitiveAttribute == primitive->attributes.end())
      return false;

    int dracoAttributeIndex = attribute.second.Get<int>();
    const auto pAttribute = mesh->GetAttributeByUniqueId(dracoAttributeIndex);
    const auto pBuffer = pAttribute->buffer();
    const auto componentType = model->accessors[primitiveAttribute->second].componentType;

    // Create a new buffer for this decoded buffer
    Buffer decodedBuffer;
    size_t bufferSize = mesh->num_points() * pAttribute->num_components() * GetComponentSizeInBytes(componentType);
    decodedBuffer.data.resize(bufferSize);

    if (!GetAttributeForAllPoints(componentType, mesh.get(), pAttribute, decodedBuffer.data))
      return false;

    model->buffers.emplace_back(std::move(decodedBuffer));

    BufferView decodedBufferView;
    decodedBufferView.buffer = int(model->buffers.size() - 1);
    decodedBufferView.byteLength = bufferSize;
    decodedBufferView.byteOffset = pAttribute->byte_offset();
    decodedBufferView.byteStride = pAttribute->byte_stride();
    decodedBufferView.target = primitive->indices >= 0 ? TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER : TINYGLTF_TARGET_ARRAY_BUFFER;
    model->bufferViews.emplace_back(std::move(decodedBufferView));

    model->accessors[primitiveAttribute->second].bufferView = int(model->bufferViews.size() - 1);
    model->accessors[primitiveAttribute->second].count = int(mesh->num_points());
  }

  return true;
}
#endif

static bool ParsePrimitive(Primitive *primitive, Model *model, std::string *err,
                           const json &o) {
  double material = -1.0;
  ParseNumberProperty(&material, err, o, "material", false);
  primitive->material = static_cast<int>(material);

  double mode = static_cast<double>(TINYGLTF_MODE_TRIANGLES);
  ParseNumberProperty(&mode, err, o, "mode", false);

  int primMode = static_cast<int>(mode);
  primitive->mode = primMode;  // Why only triangled were supported ?

  double indices = -1.0;
  ParseNumberProperty(&indices, err, o, "indices", false);
  primitive->indices = static_cast<int>(indices);
  if (!ParseStringIntProperty(&primitive->attributes, err, o, "attributes",
                              true, "Primitive")) {
    return false;
  }

  // Look for morph targets
  json::const_iterator targetsObject = o.find("targets");
  if ((targetsObject != o.end()) && targetsObject.value().is_array()) {
    for (json::const_iterator i = targetsObject.value().begin();
         i != targetsObject.value().end(); i++) {
      std::map<std::string, int> targetAttribues;

      const json &dict = i.value();
      json::const_iterator dictIt(dict.begin());
      json::const_iterator dictItEnd(dict.end());

      for (; dictIt != dictItEnd; ++dictIt) {
        targetAttribues[dictIt.key()] = static_cast<int>(dictIt.value());
      }
      primitive->targets.push_back(targetAttribues);
    }
  }

  ParseExtrasProperty(&(primitive->extras), o);

  ParseExtensionsProperty(&primitive->extensions, err, o);

#ifdef TINYGLTF_ENABLE_DRACO
  auto dracoExtension = primitive->extensions.find("KHR_draco_mesh_compression");
  if (dracoExtension != primitive->extensions.end())
  {
      ParseDracoExtension(primitive, model, err, dracoExtension->second);
  }
#endif

  return true;
}

static bool ParseMesh(Mesh *mesh, Model *model, std::string *err, const json &o) {
  ParseStringProperty(&mesh->name, err, o, "name", false);

  mesh->primitives.clear();
  json::const_iterator primObject = o.find("primitives");
  if ((primObject != o.end()) && primObject.value().is_array()) {
    for (json::const_iterator i = primObject.value().begin();
         i != primObject.value().end(); i++) {
      Primitive primitive;
      if (ParsePrimitive(&primitive, model, err, i.value())) {
        // Only add the primitive if the parsing succeeds.
        mesh->primitives.push_back(primitive);
      }
    }
  }

  // Look for morph targets
  json::const_iterator targetsObject = o.find("targets");
  if ((targetsObject != o.end()) && targetsObject.value().is_array()) {
    for (json::const_iterator i = targetsObject.value().begin();
         i != targetsObject.value().end(); i++) {
      std::map<std::string, int> targetAttribues;

      const json &dict = i.value();
      json::const_iterator dictIt(dict.begin());
      json::const_iterator dictItEnd(dict.end());

      for (; dictIt != dictItEnd; ++dictIt) {
        targetAttribues[dictIt.key()] = static_cast<int>(dictIt.value());
      }
      mesh->targets.push_back(targetAttribues);
    }
  }

  // Should probably check if has targets and if dimensions fit
  ParseNumberArrayProperty(&mesh->weights, err, o, "weights", false);

  ParseExtensionsProperty(&mesh->extensions, err, o);
  ParseExtrasProperty(&(mesh->extras), o);

  return true;
}

static bool ParseLight(Light *light, std::string *err, const json &o) {
  ParseStringProperty(&light->name, err, o, "name", false);
  ParseNumberArrayProperty(&light->color, err, o, "color", false);
  ParseStringProperty(&light->type, err, o, "type", false);
  return true;
}

static bool ParseNode(Node *node, std::string *err, const json &o) {
  ParseStringProperty(&node->name, err, o, "name", false);

  double skin = -1.0;
  ParseNumberProperty(&skin, err, o, "skin", false);
  node->skin = static_cast<int>(skin);

  // Matrix and T/R/S are exclusive
  if (!ParseNumberArrayProperty(&node->matrix, err, o, "matrix", false)) {
    ParseNumberArrayProperty(&node->rotation, err, o, "rotation", false);
    ParseNumberArrayProperty(&node->scale, err, o, "scale", false);
    ParseNumberArrayProperty(&node->translation, err, o, "translation", false);
  }

  double camera = -1.0;
  ParseNumberProperty(&camera, err, o, "camera", false);
  node->camera = static_cast<int>(camera);

  double mesh = -1.0;
  ParseNumberProperty(&mesh, err, o, "mesh", false);
  node->mesh = int(mesh);

  node->children.clear();
  json::const_iterator childrenObject = o.find("children");
  if ((childrenObject != o.end()) && childrenObject.value().is_array()) {
    for (json::const_iterator i = childrenObject.value().begin();
         i != childrenObject.value().end(); i++) {
      if (!i.value().is_number()) {
        if (err) {
          (*err) += "Invalid `children` array.\n";
        }
        return false;
      }
      const int &childrenNode = static_cast<int>(i.value());
      node->children.push_back(childrenNode);
    }
  }

  ParseExtensionsProperty(&node->extensions, err, o);
  ParseExtrasProperty(&(node->extras), o);

  return true;
}

static bool ParseMaterial(Material *material, std::string *err, const json &o) {
  material->values.clear();
  material->extensions.clear();
  material->additionalValues.clear();

  json::const_iterator it(o.begin());
  json::const_iterator itEnd(o.end());

  for (; it != itEnd; it++) {
    if (it.key() == "pbrMetallicRoughness") {
      if (it.value().is_object()) {
        const json &values_object = it.value();

        json::const_iterator itVal(values_object.begin());
        json::const_iterator itValEnd(values_object.end());

        for (; itVal != itValEnd; itVal++) {
          Parameter param;
          if (ParseParameterProperty(&param, err, values_object, itVal.key(),
                                     false)) {
            material->values[itVal.key()] = param;
          }
        }
      }
    } else if (it.key() == "extensions" || it.key() == "extras") {
      // done later, skip, otherwise poorly parsed contents will be saved in the
      // parametermap and serialized again later
    } else {
      Parameter param;
      if (ParseParameterProperty(&param, err, o, it.key(), false)) {
        material->additionalValues[it.key()] = param;
      }
    }
  }

  ParseExtensionsProperty(&material->extensions, err, o);
  ParseExtrasProperty(&(material->extras), o);

  return true;
}

static bool ParseAnimationChannel(AnimationChannel *channel, std::string *err,
                                  const json &o) {
  double samplerIndex = -1.0;
  double targetIndex = -1.0;
  if (!ParseNumberProperty(&samplerIndex, err, o, "sampler", true,
                           "AnimationChannel")) {
    if (err) {
      (*err) += "`sampler` field is missing in animation channels\n";
    }
    return false;
  }

  json::const_iterator targetIt = o.find("target");
  if ((targetIt != o.end()) && targetIt.value().is_object()) {
    const json &target_object = targetIt.value();

    if (!ParseNumberProperty(&targetIndex, err, target_object, "node", true)) {
      if (err) {
        (*err) += "`node` field is missing in animation.channels.target\n";
      }
      return false;
    }

    if (!ParseStringProperty(&channel->target_path, err, target_object, "path",
                             true)) {
      if (err) {
        (*err) += "`path` field is missing in animation.channels.target\n";
      }
      return false;
    }
  }

  channel->sampler = static_cast<int>(samplerIndex);
  channel->target_node = static_cast<int>(targetIndex);

  ParseExtrasProperty(&(channel->extras), o);

  return true;
}

static bool ParseAnimation(Animation *animation, std::string *err,
                           const json &o) {
  {
    json::const_iterator channelsIt = o.find("channels");
    if ((channelsIt != o.end()) && channelsIt.value().is_array()) {
      for (json::const_iterator i = channelsIt.value().begin();
           i != channelsIt.value().end(); i++) {
        AnimationChannel channel;
        if (ParseAnimationChannel(&channel, err, i.value())) {
          // Only add the channel if the parsing succeeds.
          animation->channels.push_back(channel);
        }
      }
    }
  }

  {
    json::const_iterator samplerIt = o.find("samplers");
    if ((samplerIt != o.end()) && samplerIt.value().is_array()) {
      const json &sampler_array = samplerIt.value();

      json::const_iterator it = sampler_array.begin();
      json::const_iterator itEnd = sampler_array.end();

      for (; it != itEnd; it++) {
        const json &s = it->get<json>();

        AnimationSampler sampler;
        double inputIndex = -1.0;
        double outputIndex = -1.0;
        if (!ParseNumberProperty(&inputIndex, err, s, "input", true)) {
          if (err) {
            (*err) += "`input` field is missing in animation.sampler\n";
          }
          return false;
        }
        ParseStringProperty(&sampler.interpolation, err, s, "interpolation",
                            false);
        if (!ParseNumberProperty(&outputIndex, err, s, "output", true)) {
          if (err) {
            (*err) += "`output` field is missing in animation.sampler\n";
          }
          return false;
        }
        sampler.input = static_cast<int>(inputIndex);
        sampler.output = static_cast<int>(outputIndex);
        ParseExtrasProperty(&(sampler.extras), s);
        animation->samplers.push_back(sampler);
      }
    }
  }

  ParseStringProperty(&animation->name, err, o, "name", false);

  ParseExtrasProperty(&(animation->extras), o);

  return true;
}

static bool ParseSampler(Sampler *sampler, std::string *err, const json &o) {
  ParseStringProperty(&sampler->name, err, o, "name", false);

  double minFilter =
      static_cast<double>(TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR);
  double magFilter = static_cast<double>(TINYGLTF_TEXTURE_FILTER_LINEAR);
  double wrapS = static_cast<double>(TINYGLTF_TEXTURE_WRAP_REPEAT);
  double wrapT = static_cast<double>(TINYGLTF_TEXTURE_WRAP_REPEAT);
  ParseNumberProperty(&minFilter, err, o, "minFilter", false);
  ParseNumberProperty(&magFilter, err, o, "magFilter", false);
  ParseNumberProperty(&wrapS, err, o, "wrapS", false);
  ParseNumberProperty(&wrapT, err, o, "wrapT", false);

  sampler->minFilter = static_cast<int>(minFilter);
  sampler->magFilter = static_cast<int>(magFilter);
  sampler->wrapS = static_cast<int>(wrapS);
  sampler->wrapT = static_cast<int>(wrapT);

  ParseExtrasProperty(&(sampler->extras), o);

  return true;
}

static bool ParseSkin(Skin *skin, std::string *err, const json &o) {
  ParseStringProperty(&skin->name, err, o, "name", false, "Skin");

  std::vector<double> joints;
  if (!ParseNumberArrayProperty(&joints, err, o, "joints", false, "Skin")) {
    return false;
  }

  double skeleton = -1.0;
  ParseNumberProperty(&skeleton, err, o, "skeleton", false, "Skin");
  skin->skeleton = static_cast<int>(skeleton);

  skin->joints.resize(joints.size());
  for (size_t i = 0; i < joints.size(); i++) {
    skin->joints[i] = static_cast<int>(joints[i]);
  }

  double invBind = -1.0;
  ParseNumberProperty(&invBind, err, o, "inverseBindMatrices", true, "Skin");
  skin->inverseBindMatrices = static_cast<int>(invBind);

  return true;
}

static bool ParsePerspectiveCamera(PerspectiveCamera *camera, std::string *err,
                                   const json &o) {
  double yfov = 0.0;
  if (!ParseNumberProperty(&yfov, err, o, "yfov", true, "OrthographicCamera")) {
    return false;
  }

  double znear = 0.0;
  if (!ParseNumberProperty(&znear, err, o, "znear", true,
                           "PerspectiveCamera")) {
    return false;
  }

  double aspectRatio = 0.0;  // = invalid
  ParseNumberProperty(&aspectRatio, err, o, "aspectRatio", false,
                      "PerspectiveCamera");

  double zfar = 0.0;  // = invalid
  ParseNumberProperty(&zfar, err, o, "zfar", false, "PerspectiveCamera");

  camera->aspectRatio = aspectRatio;
  camera->zfar = zfar;
  camera->yfov = yfov;
  camera->znear = znear;

  ParseExtensionsProperty(&camera->extensions, err, o);
  ParseExtrasProperty(&(camera->extras), o);

  // TODO(syoyo): Validate parameter values.

  return true;
}

static bool ParseOrthographicCamera(OrthographicCamera *camera,
                                    std::string *err, const json &o) {
  double xmag = 0.0;
  if (!ParseNumberProperty(&xmag, err, o, "xmag", true, "OrthographicCamera")) {
    return false;
  }

  double ymag = 0.0;
  if (!ParseNumberProperty(&ymag, err, o, "ymag", true, "OrthographicCamera")) {
    return false;
  }

  double zfar = 0.0;
  if (!ParseNumberProperty(&zfar, err, o, "zfar", true, "OrthographicCamera")) {
    return false;
  }

  double znear = 0.0;
  if (!ParseNumberProperty(&znear, err, o, "znear", true,
                           "OrthographicCamera")) {
    return false;
  }

  ParseExtensionsProperty(&camera->extensions, err, o);
  ParseExtrasProperty(&(camera->extras), o);

  camera->xmag = xmag;
  camera->ymag = ymag;
  camera->zfar = zfar;
  camera->znear = znear;

  // TODO(syoyo): Validate parameter values.

  return true;
}

static bool ParseCamera(Camera *camera, std::string *err, const json &o) {
  if (!ParseStringProperty(&camera->type, err, o, "type", true, "Camera")) {
    return false;
  }

  if (camera->type.compare("orthographic") == 0) {
    if (o.find("orthographic") == o.end()) {
      if (err) {
        std::stringstream ss;
        ss << "Orhographic camera description not found." << std::endl;
        (*err) += ss.str();
      }
      return false;
    }

    const json &v = o.find("orthographic").value();
    if (!v.is_object()) {
      if (err) {
        std::stringstream ss;
        ss << "\"orthographic\" is not a JSON object." << std::endl;
        (*err) += ss.str();
      }
      return false;
    }

    if (!ParseOrthographicCamera(&camera->orthographic, err, v.get<json>())) {
      return false;
    }
  } else if (camera->type.compare("perspective") == 0) {
    if (o.find("perspective") == o.end()) {
      if (err) {
        std::stringstream ss;
        ss << "Perspective camera description not found." << std::endl;
        (*err) += ss.str();
      }
      return false;
    }

    const json &v = o.find("perspective").value();
    if (!v.is_object()) {
      if (err) {
        std::stringstream ss;
        ss << "\"perspective\" is not a JSON object." << std::endl;
        (*err) += ss.str();
      }
      return false;
    }

    if (!ParsePerspectiveCamera(&camera->perspective, err, v.get<json>())) {
      return false;
    }
  } else {
    if (err) {
      std::stringstream ss;
      ss << "Invalid camera type: \"" << camera->type
         << "\". Must be \"perspective\" or \"orthographic\"" << std::endl;
      (*err) += ss.str();
    }
    return false;
  }

  ParseStringProperty(&camera->name, err, o, "name", false);

  ParseExtensionsProperty(&camera->extensions, err, o);
  ParseExtrasProperty(&(camera->extras), o);

  return true;
}

bool TinyGLTF::LoadFromString(Model *model, std::string *err, std::string *warn,
                              const char *str, unsigned int length,
                              const std::string &base_dir,
                              unsigned int check_sections) {
  if (length < 4) {
    if (err) {
      (*err) = "JSON string too short.\n";
    }
    return false;
  }

  json v;

#if (defined(__cpp_exceptions) || defined(__EXCEPTIONS) || \
     defined(_CPPUNWIND)) &&                               \
    not defined(TINYGLTF_NOEXCEPTION)
  try {
    v = json::parse(str, str + length);

  } catch (const std::exception &e) {
    if (err) {
      (*err) = e.what();
    }
    return false;
  }
#else
  {
    v = json::parse(str, str + length, nullptr, /* exception */ false);

    if (!v.is_object()) {
      // Assume parsing was failed.
      if (err) {
        (*err) = "Failed to parse JSON object\n";
      }
      return false;
    }
  }
#endif

  if (!v.is_object()) {
    // root is not an object.
    if (err) {
      (*err) = "Root element is not a JSON object\n";
    }
    return false;
  }

  // scene is not mandatory.
  // FIXME Maybe a better way to handle it than removing the code

  {
    json::const_iterator it = v.find("scenes");
    if ((it != v.end()) && it.value().is_array()) {
      // OK
    } else if (check_sections & REQUIRE_SCENES) {
      if (err) {
        (*err) += "\"scenes\" object not found in .gltf or not an array type\n";
      }
      return false;
    }
  }

  {
    json::const_iterator it = v.find("nodes");
    if ((it != v.end()) && it.value().is_array()) {
      // OK
    } else if (check_sections & REQUIRE_NODES) {
      if (err) {
        (*err) += "\"nodes\" object not found in .gltf\n";
      }
      return false;
    }
  }

  {
    json::const_iterator it = v.find("accessors");
    if ((it != v.end()) && it.value().is_array()) {
      // OK
    } else if (check_sections & REQUIRE_ACCESSORS) {
      if (err) {
        (*err) += "\"accessors\" object not found in .gltf\n";
      }
      return false;
    }
  }

  {
    json::const_iterator it = v.find("buffers");
    if ((it != v.end()) && it.value().is_array()) {
      // OK
    } else if (check_sections & REQUIRE_BUFFERS) {
      if (err) {
        (*err) += "\"buffers\" object not found in .gltf\n";
      }
      return false;
    }
  }

  {
    json::const_iterator it = v.find("bufferViews");
    if ((it != v.end()) && it.value().is_array()) {
      // OK
    } else if (check_sections & REQUIRE_BUFFER_VIEWS) {
      if (err) {
        (*err) += "\"bufferViews\" object not found in .gltf\n";
      }
      return false;
    }
  }

  model->buffers.clear();
  model->bufferViews.clear();
  model->accessors.clear();
  model->meshes.clear();
  model->cameras.clear();
  model->nodes.clear();
  model->extensionsUsed.clear();
  model->extensionsRequired.clear();
  model->extensions.clear();
  model->defaultScene = -1;

  // 1. Parse Asset
  {
    json::const_iterator it = v.find("asset");
    if ((it != v.end()) && it.value().is_object()) {
      const json &root = it.value();

      ParseAsset(&model->asset, err, root);
    }
  }

  // 2. Parse extensionUsed
  {
    json::const_iterator it = v.find("extensionsUsed");
    if ((it != v.end()) && it.value().is_array()) {
      const json &root = it.value();
      for (unsigned int i = 0; i < root.size(); ++i) {
        model->extensionsUsed.push_back(root[i].get<std::string>());
      }
    }
  }

  {
    json::const_iterator it = v.find("extensionsRequired");
    if ((it != v.end()) && it.value().is_array()) {
      const json &root = it.value();
      for (unsigned int i = 0; i < root.size(); ++i) {
        model->extensionsRequired.push_back(root[i].get<std::string>());
      }
    }
  }

  // 3. Parse Buffer
  {
    json::const_iterator rootIt = v.find("buffers");
    if ((rootIt != v.end()) && rootIt.value().is_array()) {
      const json &root = rootIt.value();

      json::const_iterator it(root.begin());
      json::const_iterator itEnd(root.end());
      for (; it != itEnd; it++) {
        if (!it.value().is_object()) {
          if (err) {
            (*err) += "`buffers' does not contain an JSON object.";
          }
          return false;
        }
        Buffer buffer;
        if (!ParseBuffer(&buffer, err, it->get<json>(), &fs, base_dir,
                         is_binary_, bin_data_, bin_size_)) {
          return false;
        }

        model->buffers.push_back(buffer);
      }
    }
  }

  // 4. Parse BufferView
  {
    json::const_iterator rootIt = v.find("bufferViews");
    if ((rootIt != v.end()) && rootIt.value().is_array()) {
      const json &root = rootIt.value();

      json::const_iterator it(root.begin());
      json::const_iterator itEnd(root.end());
      for (; it != itEnd; it++) {
        if (!it.value().is_object()) {
          if (err) {
            (*err) += "`bufferViews' does not contain an JSON object.";
          }
          return false;
        }
        BufferView bufferView;
        if (!ParseBufferView(&bufferView, err, it->get<json>())) {
          return false;
        }

        model->bufferViews.push_back(bufferView);
      }
    }
  }

  // 5. Parse Accessor
  {
    json::const_iterator rootIt = v.find("accessors");
    if ((rootIt != v.end()) && rootIt.value().is_array()) {
      const json &root = rootIt.value();

      json::const_iterator it(root.begin());
      json::const_iterator itEnd(root.end());
      for (; it != itEnd; it++) {
        if (!it.value().is_object()) {
          if (err) {
            (*err) += "`accessors' does not contain an JSON object.";
          }
          return false;
        }
        Accessor accessor;
        if (!ParseAccessor(&accessor, err, it->get<json>())) {
          return false;
        }

        model->accessors.push_back(accessor);
      }
    }
  }

  // 6. Parse Mesh
  {
    json::const_iterator rootIt = v.find("meshes");
    if ((rootIt != v.end()) && rootIt.value().is_array()) {
      const json &root = rootIt.value();

      json::const_iterator it(root.begin());
      json::const_iterator itEnd(root.end());
      for (; it != itEnd; it++) {
        if (!it.value().is_object()) {
          if (err) {
            (*err) += "`meshes' does not contain an JSON object.";
          }
          return false;
        }
        Mesh mesh;
        if (!ParseMesh(&mesh, model, err, it->get<json>())) {
          return false;
        }

        model->meshes.push_back(mesh);
      }
    }
  }

  // Assign missing bufferView target types
  // - Look for missing Mesh indices
  // - Look for missing bufferView targets
  for (auto &mesh : model->meshes)
  {
    for (auto &primitive : mesh.primitives)
    {
      if (primitive.indices > -1) // has indices from parsing step, must be Element Array Buffer
      {
        model->bufferViews[model->accessors[primitive.indices].bufferView]
            .target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
        // we could optionally check if acessors' bufferView type is Scalar, as it should be
      }
    }
  }
  // find any missing targets, must be an array buffer type if not fulfilled from previous check
  for (auto &bufferView : model->bufferViews)
  {
    if (bufferView.target == 0) // missing target type
    {
      bufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
    }
  }

  // 7. Parse Node
  {
    json::const_iterator rootIt = v.find("nodes");
    if ((rootIt != v.end()) && rootIt.value().is_array()) {
      const json &root = rootIt.value();

      json::const_iterator it(root.begin());
      json::const_iterator itEnd(root.end());
      for (; it != itEnd; it++) {
        if (!it.value().is_object()) {
          if (err) {
            (*err) += "`nodes' does not contain an JSON object.";
          }
          return false;
        }
        Node node;
        if (!ParseNode(&node, err, it->get<json>())) {
          return false;
        }

        model->nodes.push_back(node);
      }
    }
  }

  // 8. Parse scenes.
  {
    json::const_iterator rootIt = v.find("scenes");
    if ((rootIt != v.end()) && rootIt.value().is_array()) {
      const json &root = rootIt.value();

      json::const_iterator it(root.begin());
      json::const_iterator itEnd(root.end());
      for (; it != itEnd; it++) {
        if (!(it.value().is_object())) {
          if (err) {
            (*err) += "`scenes' does not contain an JSON object.";
          }
          return false;
        }
        const json &o = it->get<json>();
        std::vector<double> nodes;
        if (!ParseNumberArrayProperty(&nodes, err, o, "nodes", false)) {
          return false;
        }

        Scene scene;
        ParseStringProperty(&scene.name, err, o, "name", false);
        std::vector<int> nodesIds;
        for (size_t i = 0; i < nodes.size(); i++) {
          nodesIds.push_back(static_cast<int>(nodes[i]));
        }
        scene.nodes = nodesIds;

        ParseExtensionsProperty(&scene.extensions, err, o);
        ParseExtrasProperty(&scene.extras, o);

        model->scenes.push_back(scene);
      }
    }
  }

  // 9. Parse default scenes.
  {
    json::const_iterator rootIt = v.find("scene");
    if ((rootIt != v.end()) && rootIt.value().is_number()) {
      const int defaultScene = rootIt.value();

      model->defaultScene = static_cast<int>(defaultScene);
    }
  }

  // 10. Parse Material
  {
    json::const_iterator rootIt = v.find("materials");
    if ((rootIt != v.end()) && rootIt.value().is_array()) {
      const json &root = rootIt.value();

      json::const_iterator it(root.begin());
      json::const_iterator itEnd(root.end());
      for (; it != itEnd; it++) {
        if (!it.value().is_object()) {
          if (err) {
            (*err) += "`materials' does not contain an JSON object.";
          }
          return false;
        }
        json jsonMaterial = it->get<json>();

        Material material;
        ParseStringProperty(&material.name, err, jsonMaterial, "name", false);

        if (!ParseMaterial(&material, err, jsonMaterial)) {
          return false;
        }

        model->materials.push_back(material);
      }
    }
  }

  // 11. Parse Image
  {
    json::const_iterator rootIt = v.find("images");
    if ((rootIt != v.end()) && rootIt.value().is_array()) {
      const json &root = rootIt.value();

      json::const_iterator it(root.begin());
      json::const_iterator itEnd(root.end());
      int idx = 0;
      for (; it != itEnd; it++, idx++) {
        if (!it.value().is_object()) {
          if (err) {
            (*err) += "image[" + std::to_string(idx) + "] is not a JSON object.";
          }
          return false;
        }
        Image image;
        if (!ParseImage(&image, idx, err, warn, it.value(), base_dir, &fs,
                        &this->LoadImageData, load_image_user_data_)) {
          return false;
        }

        if (image.bufferView != -1) {
          // Load image from the buffer view.
          if (size_t(image.bufferView) >= model->bufferViews.size()) {
            if (err) {
              std::stringstream ss;
              ss << "image[" << idx << "] bufferView \"" << image.bufferView
                 << "\" not found in the scene." << std::endl;
              (*err) += ss.str();
            }
            return false;
          }

          const BufferView &bufferView =
              model->bufferViews[size_t(image.bufferView)];
          const Buffer &buffer = model->buffers[size_t(bufferView.buffer)];

          if (*LoadImageData == nullptr) {
            if (err) {
              (*err) += "No LoadImageData callback specified.\n";
            }
            return false;
          }
          bool ret = LoadImageData(&image, idx, err, warn, image.width, image.height,
                                   &buffer.data[bufferView.byteOffset],
                                   static_cast<int>(bufferView.byteLength),
                                   load_image_user_data_);
          if (!ret) {
            return false;
          }
        }

        model->images.push_back(image);
      }
    }
  }

  // 12. Parse Texture
  {
    json::const_iterator rootIt = v.find("textures");
    if ((rootIt != v.end()) && rootIt.value().is_array()) {
      const json &root = rootIt.value();

      json::const_iterator it(root.begin());
      json::const_iterator itEnd(root.end());
      for (; it != itEnd; it++) {
        if (!it.value().is_object()) {
          if (err) {
            (*err) += "`textures' does not contain an JSON object.";
          }
          return false;
        }
        Texture texture;
        if (!ParseTexture(&texture, err, it->get<json>(), base_dir)) {
          return false;
        }

        model->textures.push_back(texture);
      }
    }
  }

  // 13. Parse Animation
  {
    json::const_iterator rootIt = v.find("animations");
    if ((rootIt != v.end()) && rootIt.value().is_array()) {
      const json &root = rootIt.value();

      json::const_iterator it(root.begin());
      json::const_iterator itEnd(root.end());
      for (; it != itEnd; ++it) {
        if (!it.value().is_object()) {
          if (err) {
            (*err) += "`animations' does not contain an JSON object.";
          }
          return false;
        }
        Animation animation;
        if (!ParseAnimation(&animation, err, it->get<json>())) {
          return false;
        }

        model->animations.push_back(animation);
      }
    }
  }

  // 14. Parse Skin
  {
    json::const_iterator rootIt = v.find("skins");
    if ((rootIt != v.end()) && rootIt.value().is_array()) {
      const json &root = rootIt.value();

      json::const_iterator it(root.begin());
      json::const_iterator itEnd(root.end());
      for (; it != itEnd; ++it) {
        if (!it.value().is_object()) {
          if (err) {
            (*err) += "`skins' does not contain an JSON object.";
          }
          return false;
        }
        Skin skin;
        if (!ParseSkin(&skin, err, it->get<json>())) {
          return false;
        }

        model->skins.push_back(skin);
      }
    }
  }

  // 15. Parse Sampler
  {
    json::const_iterator rootIt = v.find("samplers");
    if ((rootIt != v.end()) && rootIt.value().is_array()) {
      const json &root = rootIt.value();

      json::const_iterator it(root.begin());
      json::const_iterator itEnd(root.end());
      for (; it != itEnd; ++it) {
        if (!it.value().is_object()) {
          if (err) {
            (*err) += "`samplers' does not contain an JSON object.";
          }
          return false;
        }
        Sampler sampler;
        if (!ParseSampler(&sampler, err, it->get<json>())) {
          return false;
        }

        model->samplers.push_back(sampler);
      }
    }
  }

  // 16. Parse Camera
  {
    json::const_iterator rootIt = v.find("cameras");
    if ((rootIt != v.end()) && rootIt.value().is_array()) {
      const json &root = rootIt.value();

      json::const_iterator it(root.begin());
      json::const_iterator itEnd(root.end());
      for (; it != itEnd; ++it) {
        if (!it.value().is_object()) {
          if (err) {
            (*err) += "`cameras' does not contain an JSON object.";
          }
          return false;
        }
        Camera camera;
        if (!ParseCamera(&camera, err, it->get<json>())) {
          return false;
        }

        model->cameras.push_back(camera);
      }
    }
  }

  // 17. Parse Extensions
  ParseExtensionsProperty(&model->extensions, err, v);

  // 18. Specific extension implementations
  {
    json::const_iterator rootIt = v.find("extensions");
    if ((rootIt != v.end()) && rootIt.value().is_object()) {
      const json &root = rootIt.value();

      json::const_iterator it(root.begin());
      json::const_iterator itEnd(root.end());
      for (; it != itEnd; ++it) {
        // parse KHR_lights_cmn extension
        if ((it.key().compare("KHR_lights_cmn") == 0) &&
            it.value().is_object()) {
          const json &object = it.value();
          json::const_iterator itLight(object.find("lights"));
          json::const_iterator itLightEnd(object.end());
          if (itLight == itLightEnd) {
            continue;
          }

          if (!itLight.value().is_array()) {
            continue;
          }

          const json &lights = itLight.value();
          json::const_iterator arrayIt(lights.begin());
          json::const_iterator arrayItEnd(lights.end());
          for (; arrayIt != arrayItEnd; ++arrayIt) {
            Light light;
            if (!ParseLight(&light, err, arrayIt.value())) {
              return false;
            }
            model->lights.push_back(light);
          }
        }
      }
    }
  }

  // 19. Parse Extras
  ParseExtrasProperty(&model->extras, v);

  return true;
}

bool TinyGLTF::LoadASCIIFromString(Model *model, std::string *err,
                                   std::string *warn, const char *str,
                                   unsigned int length,
                                   const std::string &base_dir,
                                   unsigned int check_sections) {
  is_binary_ = false;
  bin_data_ = nullptr;
  bin_size_ = 0;

  return LoadFromString(model, err, warn, str, length, base_dir,
                        check_sections);
}

bool TinyGLTF::LoadASCIIFromFile(Model *model, std::string *err,
                                 std::string *warn, const std::string &filename,
                                 unsigned int check_sections) {
  std::stringstream ss;

  if (fs.ReadWholeFile == nullptr) {
    // Programmer error, assert() ?
    ss << "Failed to read file: " << filename
       << ": one or more FS callback not set" << std::endl;
    if (err) {
      (*err) = ss.str();
    }
    return false;
  }

  std::vector<unsigned char> data;
  std::string fileerr;
  bool fileread = fs.ReadWholeFile(&data, &fileerr, filename, fs.user_data);
  if (!fileread) {
    ss << "Failed to read file: " << filename << ": " << fileerr << std::endl;
    if (err) {
      (*err) = ss.str();
    }
    return false;
  }

  size_t sz = data.size();
  if (sz == 0) {
    if (err) {
      (*err) = "Empty file.";
    }
    return false;
  }

  std::string basedir = GetBaseDir(filename);

  bool ret = LoadASCIIFromString(
      model, err, warn, reinterpret_cast<const char *>(&data.at(0)),
      static_cast<unsigned int>(data.size()), basedir, check_sections);

  return ret;
}

bool TinyGLTF::LoadBinaryFromMemory(Model *model, std::string *err,
                                    std::string *warn,
                                    const unsigned char *bytes,
                                    unsigned int size,
                                    const std::string &base_dir,
                                    unsigned int check_sections) {
  if (size < 20) {
    if (err) {
      (*err) = "Too short data size for glTF Binary.";
    }
    return false;
  }

  if (bytes[0] == 'g' && bytes[1] == 'l' && bytes[2] == 'T' &&
      bytes[3] == 'F') {
    // ok
  } else {
    if (err) {
      (*err) = "Invalid magic.";
    }
    return false;
  }

  unsigned int version;       // 4 bytes
  unsigned int length;        // 4 bytes
  unsigned int model_length;  // 4 bytes
  unsigned int model_format;  // 4 bytes;

  // @todo { Endian swap for big endian machine. }
  memcpy(&version, bytes + 4, 4);
  swap4(&version);
  memcpy(&length, bytes + 8, 4);
  swap4(&length);
  memcpy(&model_length, bytes + 12, 4);
  swap4(&model_length);
  memcpy(&model_format, bytes + 16, 4);
  swap4(&model_format);

  // In case the Bin buffer is not present, the size is exactly 20 + size of
  // JSON contents,
  // so use "greater than" operator.
  if ((20 + model_length > size) || (model_length < 1) ||
      (model_format != 0x4E4F534A)) {  // 0x4E4F534A = JSON format.
    if (err) {
      (*err) = "Invalid glTF binary.";
    }
    return false;
  }

  // Extract JSON string.
  std::string jsonString(reinterpret_cast<const char *>(&bytes[20]),
                         model_length);

  is_binary_ = true;
  bin_data_ = bytes + 20 + model_length +
              8;  // 4 bytes (buffer_length) + 4 bytes(buffer_format)
  bin_size_ =
      length - (20 + model_length);  // extract header + JSON scene data.

  bool ret = LoadFromString(model, err, warn,
                            reinterpret_cast<const char *>(&bytes[20]),
                            model_length, base_dir, check_sections);
  if (!ret) {
    return ret;
  }

  return true;
}

bool TinyGLTF::LoadBinaryFromFile(Model *model, std::string *err,
                                  std::string *warn,
                                  const std::string &filename,
                                  unsigned int check_sections) {
  std::stringstream ss;

  if (fs.ReadWholeFile == nullptr) {
    // Programmer error, assert() ?
    ss << "Failed to read file: " << filename
       << ": one or more FS callback not set" << std::endl;
    if (err) {
      (*err) = ss.str();
    }
    return false;
  }

  std::vector<unsigned char> data;
  std::string fileerr;
  bool fileread = fs.ReadWholeFile(&data, &fileerr, filename, fs.user_data);
  if (!fileread) {
    ss << "Failed to read file: " << filename << ": " << fileerr << std::endl;
    if (err) {
      (*err) = ss.str();
    }
    return false;
  }

  std::string basedir = GetBaseDir(filename);

  bool ret = LoadBinaryFromMemory(model, err, warn, &data.at(0),
                                  static_cast<unsigned int>(data.size()),
                                  basedir, check_sections);

  return ret;
}

///////////////////////
// GLTF Serialization
///////////////////////

// typedef std::pair<std::string, json> json_object_pair;

template <typename T>
static void SerializeNumberProperty(const std::string &key, T number,
                                    json &obj) {
  // obj.insert(
  //    json_object_pair(key, json(static_cast<double>(number))));
  // obj[key] = static_cast<double>(number);
  obj[key] = number;
}

template <typename T>
static void SerializeNumberArrayProperty(const std::string &key,
                                         const std::vector<T> &value,
                                         json &obj) {
  json o;
  json vals;

  for (unsigned int i = 0; i < value.size(); ++i) {
    vals.push_back(static_cast<T>(value[i]));
  }
  if (!vals.is_null()) {
    obj[key] = vals;
  }
}

static void SerializeStringProperty(const std::string &key,
                                    const std::string &value, json &obj) {
  obj[key] = value;
}

static void SerializeStringArrayProperty(const std::string &key,
                                         const std::vector<std::string> &value,
                                         json &obj) {
  json o;
  json vals;

  for (unsigned int i = 0; i < value.size(); ++i) {
    vals.push_back(value[i]);
  }

  obj[key] = vals;
}

static bool ValueToJson(const Value &value, json *ret) {
  json obj;
  switch (value.Type()) {
    case NUMBER_TYPE:
      obj = json(value.Get<double>());
      break;
    case INT_TYPE:
      obj = json(value.Get<int>());
      break;
    case BOOL_TYPE:
      obj = json(value.Get<bool>());
      break;
    case STRING_TYPE:
      obj = json(value.Get<std::string>());
      break;
    case ARRAY_TYPE: {
      for (unsigned int i = 0; i < value.ArrayLen(); ++i) {
        Value elementValue = value.Get(int(i));
        json elementJson;
        if (ValueToJson(value.Get(int(i)), &elementJson))
          obj.push_back(elementJson);
      }
      break;
    }
    case BINARY_TYPE:
      // TODO
      // obj = json(value.Get<std::vector<unsigned char>>());
      return false;
      break;
    case OBJECT_TYPE: {
      Value::Object objMap = value.Get<Value::Object>();
      for (auto &it : objMap) {
        json elementJson;
        if (ValueToJson(it.second, &elementJson)) obj[it.first] = elementJson;
      }
      break;
    }
    case NULL_TYPE:
    default:
      return false;
  }
  if (ret) *ret = obj;
  return true;
}

static void SerializeValue(const std::string &key, const Value &value,
                           json &obj) {
  json ret;
  if (ValueToJson(value, &ret)) obj[key] = ret;
}

static void SerializeGltfBufferData(const std::vector<unsigned char> &data,
                                    json &o) {
  std::string header = "data:application/octet-stream;base64,";
  std::string encodedData =
      base64_encode(&data[0], static_cast<unsigned int>(data.size()));
  SerializeStringProperty("uri", header + encodedData, o);
}

static bool SerializeGltfBufferData(const std::vector<unsigned char> &data,
                                    const std::string &binFilename) {
  std::ofstream output(binFilename.c_str(), std::ofstream::binary);
  if(!output.is_open()) return false;
  output.write(reinterpret_cast<const char *>(&data[0]),
               std::streamsize(data.size()));
  output.close();
  return true;
}

static void SerializeParameterMap(ParameterMap &param, json &o) {
  for (ParameterMap::iterator paramIt = param.begin(); paramIt != param.end();
       ++paramIt) {
    if (paramIt->second.number_array.size()) {
      SerializeNumberArrayProperty<double>(paramIt->first,
                                           paramIt->second.number_array, o);
    } else if (paramIt->second.json_double_value.size()) {
      json json_double_value;
      for (std::map<std::string, double>::iterator it =
               paramIt->second.json_double_value.begin();
           it != paramIt->second.json_double_value.end(); ++it) {
        if (it->first == "index") {
          json_double_value[it->first] = paramIt->second.TextureIndex();
        } else {
          json_double_value[it->first] = it->second;
        }
      }

      o[paramIt->first] = json_double_value;
    } else if (!paramIt->second.string_value.empty()) {
      SerializeStringProperty(paramIt->first, paramIt->second.string_value, o);
    } else if (paramIt->second.has_number_value) {
      o[paramIt->first] = paramIt->second.number_value;
    } else {
      o[paramIt->first] = paramIt->second.bool_value;
    }
  }
}

static void SerializeExtensionMap(ExtensionMap &extensions, json &o) {
  if (!extensions.size()) return;

  json extMap;
  for (ExtensionMap::iterator extIt = extensions.begin();
       extIt != extensions.end(); ++extIt) {
    json extension_values;

    // Allow an empty object for extension(#97)
    json ret;
    if (ValueToJson(extIt->second, &ret)) {
      extMap[extIt->first] = ret;
    }
    if(ret.is_null()) {
      if (!(extIt->first.empty())) { // name should not be empty, but for sure
        // create empty object so that an extension name is still included in json.
        extMap[extIt->first] = json({});
      }
    }
  }
  o["extensions"] = extMap;
}

static void SerializeGltfAccessor(Accessor &accessor, json &o) {
  SerializeNumberProperty<int>("bufferView", accessor.bufferView, o);

  if (accessor.byteOffset != 0.0)
    SerializeNumberProperty<int>("byteOffset", int(accessor.byteOffset), o);

  SerializeNumberProperty<int>("componentType", accessor.componentType, o);
  SerializeNumberProperty<size_t>("count", accessor.count, o);
  SerializeNumberArrayProperty<double>("min", accessor.minValues, o);
  SerializeNumberArrayProperty<double>("max", accessor.maxValues, o);
  std::string type;
  switch (accessor.type) {
    case TINYGLTF_TYPE_SCALAR:
      type = "SCALAR";
      break;
    case TINYGLTF_TYPE_VEC2:
      type = "VEC2";
      break;
    case TINYGLTF_TYPE_VEC3:
      type = "VEC3";
      break;
    case TINYGLTF_TYPE_VEC4:
      type = "VEC4";
      break;
    case TINYGLTF_TYPE_MAT2:
      type = "MAT2";
      break;
    case TINYGLTF_TYPE_MAT3:
      type = "MAT3";
      break;
    case TINYGLTF_TYPE_MAT4:
      type = "MAT4";
      break;
  }

  SerializeStringProperty("type", type, o);
  if (!accessor.name.empty()) SerializeStringProperty("name", accessor.name, o);

  if (accessor.extras.Type() != NULL_TYPE) {
    SerializeValue("extras", accessor.extras, o);
  }
}

static void SerializeGltfAnimationChannel(AnimationChannel &channel, json &o) {
  SerializeNumberProperty("sampler", channel.sampler, o);
  json target;
  SerializeNumberProperty("node", channel.target_node, target);
  SerializeStringProperty("path", channel.target_path, target);

  o["target"] = target;

  if (channel.extras.Type() != NULL_TYPE) {
    SerializeValue("extras", channel.extras, o);
  }
}

static void SerializeGltfAnimationSampler(AnimationSampler &sampler, json &o) {
  SerializeNumberProperty("input", sampler.input, o);
  SerializeNumberProperty("output", sampler.output, o);
  SerializeStringProperty("interpolation", sampler.interpolation, o);

  if (sampler.extras.Type() != NULL_TYPE) {
    SerializeValue("extras", sampler.extras, o);
  }
}

static void SerializeGltfAnimation(Animation &animation, json &o) {
  if (!animation.name.empty())
    SerializeStringProperty("name", animation.name, o);
  json channels;
  for (unsigned int i = 0; i < animation.channels.size(); ++i) {
    json channel;
    AnimationChannel gltfChannel = animation.channels[i];
    SerializeGltfAnimationChannel(gltfChannel, channel);
    channels.push_back(channel);
  }
  o["channels"] = channels;

  json samplers;
  for (unsigned int i = 0; i < animation.samplers.size(); ++i) {
    json sampler;
    AnimationSampler gltfSampler = animation.samplers[i];
    SerializeGltfAnimationSampler(gltfSampler, sampler);
    samplers.push_back(sampler);
  }

  o["samplers"] = samplers;

  if (animation.extras.Type() != NULL_TYPE) {
    SerializeValue("extras", animation.extras, o);
  }
}

static void SerializeGltfAsset(Asset &asset, json &o) {
  if (!asset.generator.empty()) {
    SerializeStringProperty("generator", asset.generator, o);
  }

  if (!asset.version.empty()) {
    SerializeStringProperty("version", asset.version, o);
  }

  if (asset.extras.Keys().size()) {
    SerializeValue("extras", asset.extras, o);
  }

  SerializeExtensionMap(asset.extensions, o);
}

static void SerializeGltfBuffer(Buffer &buffer, json &o) {
  SerializeNumberProperty("byteLength", buffer.data.size(), o);
  SerializeGltfBufferData(buffer.data, o);

  if (buffer.name.size()) SerializeStringProperty("name", buffer.name, o);

  if (buffer.extras.Type() != NULL_TYPE) {
    SerializeValue("extras", buffer.extras, o);
  }
}

static bool SerializeGltfBuffer(Buffer &buffer, json &o,
                                const std::string &binFilename,
                                const std::string &binBaseFilename) {
  if(!SerializeGltfBufferData(buffer.data, binFilename)) return false;
  SerializeNumberProperty("byteLength", buffer.data.size(), o);
  SerializeStringProperty("uri", binBaseFilename, o);

  if (buffer.name.size()) SerializeStringProperty("name", buffer.name, o);

  if (buffer.extras.Type() != NULL_TYPE) {
    SerializeValue("extras", buffer.extras, o);
  }
  return true;
}

static void SerializeGltfBufferView(BufferView &bufferView, json &o) {
  SerializeNumberProperty("buffer", bufferView.buffer, o);
  SerializeNumberProperty<size_t>("byteLength", bufferView.byteLength, o);

  // byteStride is optional, minimum allowed is 4
  if (bufferView.byteStride >= 4) {
    SerializeNumberProperty<size_t>("byteStride", bufferView.byteStride, o);
  }
  // byteOffset is optional, default is 0
  if (bufferView.byteOffset > 0) {
    SerializeNumberProperty<size_t>("byteOffset", bufferView.byteOffset, o);
  }
  // Target is optional, check if it contains a valid value
  if (bufferView.target == TINYGLTF_TARGET_ARRAY_BUFFER ||
      bufferView.target == TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER) {
    SerializeNumberProperty("target", bufferView.target, o);
  }
  if (bufferView.name.size()) {
    SerializeStringProperty("name", bufferView.name, o);
  }

  if (bufferView.extras.Type() != NULL_TYPE) {
    SerializeValue("extras", bufferView.extras, o);
  }
}

static void SerializeGltfImage(Image &image, json &o) {
  SerializeStringProperty("uri", image.uri, o);

  if (image.name.size()) {
    SerializeStringProperty("name", image.name, o);
  }

  if (image.extras.Type() != NULL_TYPE) {
    SerializeValue("extras", image.extras, o);
  }

  SerializeExtensionMap(image.extensions, o);
}

static void SerializeGltfMaterial(Material &material, json &o) {
  if (material.extras.Size()) SerializeValue("extras", material.extras, o);
  SerializeExtensionMap(material.extensions, o);

  if (material.values.size()) {
    json pbrMetallicRoughness;
    SerializeParameterMap(material.values, pbrMetallicRoughness);
    o["pbrMetallicRoughness"] = pbrMetallicRoughness;
  }

  SerializeParameterMap(material.additionalValues, o);

  if (material.name.size()) {
    SerializeStringProperty("name", material.name, o);
  }

  if (material.extras.Type() != NULL_TYPE) {
    SerializeValue("extras", material.extras, o);
  }
}

static void SerializeGltfMesh(Mesh &mesh, json &o) {
  json primitives;
  for (unsigned int i = 0; i < mesh.primitives.size(); ++i) {
    json primitive;
    json attributes;
    Primitive gltfPrimitive = mesh.primitives[i];
    for (std::map<std::string, int>::iterator attrIt =
             gltfPrimitive.attributes.begin();
         attrIt != gltfPrimitive.attributes.end(); ++attrIt) {
      SerializeNumberProperty<int>(attrIt->first, attrIt->second, attributes);
    }

    primitive["attributes"] = attributes;

    // Indicies is optional
    if (gltfPrimitive.indices > -1) {
      SerializeNumberProperty<int>("indices", gltfPrimitive.indices, primitive);
    }
    // Material is optional
    if (gltfPrimitive.material > -1) {
      SerializeNumberProperty<int>("material", gltfPrimitive.material,
                                   primitive);
    }
    SerializeNumberProperty<int>("mode", gltfPrimitive.mode, primitive);

    // Morph targets
    if (gltfPrimitive.targets.size()) {
      json targets;
      for (unsigned int k = 0; k < gltfPrimitive.targets.size(); ++k) {
        json targetAttributes;
        std::map<std::string, int> targetData = gltfPrimitive.targets[k];
        for (std::map<std::string, int>::iterator attrIt = targetData.begin();
             attrIt != targetData.end(); ++attrIt) {
          SerializeNumberProperty<int>(attrIt->first, attrIt->second,
                                       targetAttributes);
        }

        targets.push_back(targetAttributes);
      }
      primitive["targets"] = targets;
    }

    if (gltfPrimitive.extras.Type() != NULL_TYPE) {
      SerializeValue("extras", gltfPrimitive.extras, primitive);
    }

    primitives.push_back(primitive);
  }

  o["primitives"] = primitives;
  if (mesh.weights.size()) {
    SerializeNumberArrayProperty<double>("weights", mesh.weights, o);
  }

  if (mesh.name.size()) {
    SerializeStringProperty("name", mesh.name, o);
  }

  if (mesh.extras.Type() != NULL_TYPE) {
    SerializeValue("extras", mesh.extras, o);
  }
}

static void SerializeGltfLight(Light &light, json &o) {
  if (!light.name.empty()) SerializeStringProperty("name", light.name, o);
  SerializeNumberArrayProperty("color", light.color, o);
  SerializeStringProperty("type", light.type, o);
}

static void SerializeGltfNode(Node &node, json &o) {
  if (node.translation.size() > 0) {
    SerializeNumberArrayProperty<double>("translation", node.translation, o);
  }
  if (node.rotation.size() > 0) {
    SerializeNumberArrayProperty<double>("rotation", node.rotation, o);
  }
  if (node.scale.size() > 0) {
    SerializeNumberArrayProperty<double>("scale", node.scale, o);
  }
  if (node.matrix.size() > 0) {
    SerializeNumberArrayProperty<double>("matrix", node.matrix, o);
  }
  if (node.mesh != -1) {
    SerializeNumberProperty<int>("mesh", node.mesh, o);
  }

  if (node.skin != -1) {
    SerializeNumberProperty<int>("skin", node.skin, o);
  }

  if (node.camera != -1) {
    SerializeNumberProperty<int>("camera", node.camera, o);
  }

  if (node.extras.Type() != NULL_TYPE) {
    SerializeValue("extras", node.extras, o);
  }

  SerializeExtensionMap(node.extensions, o);
  if (!node.name.empty()) SerializeStringProperty("name", node.name, o);
  SerializeNumberArrayProperty<int>("children", node.children, o);
}

static void SerializeGltfSampler(Sampler &sampler, json &o) {
  SerializeNumberProperty("magFilter", sampler.magFilter, o);
  SerializeNumberProperty("minFilter", sampler.minFilter, o);
  SerializeNumberProperty("wrapR", sampler.wrapR, o);
  SerializeNumberProperty("wrapS", sampler.wrapS, o);
  SerializeNumberProperty("wrapT", sampler.wrapT, o);

  if (sampler.extras.Type() != NULL_TYPE) {
    SerializeValue("extras", sampler.extras, o);
  }
}

static void SerializeGltfOrthographicCamera(const OrthographicCamera &camera,
                                            json &o) {
  SerializeNumberProperty("zfar", camera.zfar, o);
  SerializeNumberProperty("znear", camera.znear, o);
  SerializeNumberProperty("xmag", camera.xmag, o);
  SerializeNumberProperty("ymag", camera.ymag, o);

  if (camera.extras.Type() != NULL_TYPE) {
    SerializeValue("extras", camera.extras, o);
  }
}

static void SerializeGltfPerspectiveCamera(const PerspectiveCamera &camera,
                                           json &o) {
  SerializeNumberProperty("zfar", camera.zfar, o);
  SerializeNumberProperty("znear", camera.znear, o);
  if (camera.aspectRatio > 0) {
    SerializeNumberProperty("aspectRatio", camera.aspectRatio, o);
  }

  if (camera.yfov > 0) {
    SerializeNumberProperty("yfov", camera.yfov, o);
  }

  if (camera.extras.Type() != NULL_TYPE) {
    SerializeValue("extras", camera.extras, o);
  }
}

static void SerializeGltfCamera(const Camera &camera, json &o) {
  SerializeStringProperty("type", camera.type, o);
  if (!camera.name.empty()) {
    SerializeStringProperty("name", camera.name, o);
  }

  if (camera.type.compare("orthographic") == 0) {
    json orthographic;
    SerializeGltfOrthographicCamera(camera.orthographic, orthographic);
    o["orthographic"] = orthographic;
  } else if (camera.type.compare("perspective") == 0) {
    json perspective;
    SerializeGltfPerspectiveCamera(camera.perspective, perspective);
    o["perspective"] = perspective;
  } else {
    // ???
  }
}

static void SerializeGltfScene(Scene &scene, json &o) {
  SerializeNumberArrayProperty<int>("nodes", scene.nodes, o);

  if (scene.name.size()) {
    SerializeStringProperty("name", scene.name, o);
  }
  if (scene.extras.Type() != NULL_TYPE) {
    SerializeValue("extras", scene.extras, o);
  }
  SerializeExtensionMap(scene.extensions, o);
}

static void SerializeGltfSkin(Skin &skin, json &o) {
  if (skin.inverseBindMatrices != -1)
    SerializeNumberProperty("inverseBindMatrices", skin.inverseBindMatrices, o);

  SerializeNumberArrayProperty<int>("joints", skin.joints, o);
  SerializeNumberProperty("skeleton", skin.skeleton, o);
  if (skin.name.size()) {
    SerializeStringProperty("name", skin.name, o);
  }
}

static void SerializeGltfTexture(Texture &texture, json &o) {
  if (texture.sampler > -1) {
    SerializeNumberProperty("sampler", texture.sampler, o);
  }
  if (texture.source > -1) {
    SerializeNumberProperty("source", texture.source, o);
  }
  if (texture.extras.Type() != NULL_TYPE) {
    SerializeValue("extras", texture.extras, o);
  }
  SerializeExtensionMap(texture.extensions, o);
}

static bool WriteGltfFile(const std::string &output,
                          const std::string &content) {
  std::ofstream gltfFile(output.c_str());
  if (!gltfFile.is_open()) return false;
  gltfFile << content << std::endl;
  return true;
}

static void WriteBinaryGltfFile(const std::string &output,
                                const std::string &content) {
  std::ofstream gltfFile(output.c_str(), std::ios::binary);

  const std::string header = "glTF";
  const int version = 2;
  const int padding_size = content.size() % 4;

  // 12 bytes for header, JSON content length, 8 bytes for JSON chunk info, padding
  const int length = 12 + 8 + int(content.size()) + padding_size;
  
  gltfFile.write(header.c_str(), header.size());
  gltfFile.write(reinterpret_cast<const char *>(&version), sizeof(version));
  gltfFile.write(reinterpret_cast<const char *>(&length), sizeof(length));

  // JSON chunk info, then JSON data
  const int model_length = int(content.size()) + padding_size;
  const int model_format = 0x4E4F534A;
  gltfFile.write(reinterpret_cast<const char *>(&model_length), sizeof(model_length));
  gltfFile.write(reinterpret_cast<const char *>(&model_format), sizeof(model_format));
  gltfFile.write(content.c_str(), content.size());

  // Chunk must be multiplies of 4, so pad with spaces
  if (padding_size > 0) {
    const std::string padding = std::string(padding_size, ' ');
    gltfFile.write(padding.c_str(), padding.size());
  }
}

bool TinyGLTF::WriteGltfSceneToFile(Model *model, const std::string &filename,
                                    bool embedImages = false,
                                    bool embedBuffers = false,
                                    bool prettyPrint = true,
                                    bool writeBinary = false) {
  json output;

  // ACCESSORS
  json accessors;
  for (unsigned int i = 0; i < model->accessors.size(); ++i) {
    json accessor;
    SerializeGltfAccessor(model->accessors[i], accessor);
    accessors.push_back(accessor);
  }
  output["accessors"] = accessors;

  // ANIMATIONS
  if (model->animations.size()) {
    json animations;
    for (unsigned int i = 0; i < model->animations.size(); ++i) {
      if (model->animations[i].channels.size()) {
        json animation;
        SerializeGltfAnimation(model->animations[i], animation);
        animations.push_back(animation);
      }
    }
    output["animations"] = animations;
  }

  // ASSET
  json asset;
  SerializeGltfAsset(model->asset, asset);
  output["asset"] = asset;

  std::string defaultBinFilename = GetBaseFilename(filename);
  std::string defaultBinFileExt = ".bin";
  std::string::size_type pos = defaultBinFilename.rfind('.', defaultBinFilename.length());

  if (pos != std::string::npos) {
    defaultBinFilename = defaultBinFilename.substr(0, pos);
  }
  std::string baseDir = GetBaseDir(filename);
  if (baseDir.empty()) {
    baseDir = "./";
  }

  // BUFFERS
  std::vector<std::string> usedUris;
  json buffers;
  for (unsigned int i = 0; i < model->buffers.size(); ++i) {
    json buffer;
    if (embedBuffers) {
      SerializeGltfBuffer(model->buffers[i], buffer);
    } else {
      std::string binSavePath;
      std::string binUri;
      if (!model->buffers[i].uri.empty()
        && !IsDataURI(model->buffers[i].uri)) {
        binUri = model->buffers[i].uri;
      }
      else {
        binUri = defaultBinFilename + defaultBinFileExt;
        bool inUse = true;
        int numUsed = 0;
        while(inUse) {
          inUse = false;
          for (const std::string& usedName : usedUris) {
            if (binUri.compare(usedName) != 0) continue;
            inUse = true;
            binUri = defaultBinFilename + std::to_string(numUsed++) + defaultBinFileExt;
            break;
          }
        }
      }
      usedUris.push_back(binUri);
	  binSavePath = JoinPath(baseDir, binUri);
      if(!SerializeGltfBuffer(model->buffers[i], buffer, binSavePath,
                          binUri)) {
        return false;
      }
    }
    buffers.push_back(buffer);
  }
  output["buffers"] = buffers;

  // BUFFERVIEWS
  json bufferViews;
  for (unsigned int i = 0; i < model->bufferViews.size(); ++i) {
    json bufferView;
    SerializeGltfBufferView(model->bufferViews[i], bufferView);
    bufferViews.push_back(bufferView);
  }
  output["bufferViews"] = bufferViews;

  // Extensions used
  if (model->extensionsUsed.size()) {
    SerializeStringArrayProperty("extensionsUsed", model->extensionsUsed,
                                 output);
  }

  // Extensions required
  if (model->extensionsRequired.size()) {
    SerializeStringArrayProperty("extensionsRequired",
                                 model->extensionsRequired, output);
  }

  // IMAGES
  if (model->images.size()) {
    json images;
    for (unsigned int i = 0; i < model->images.size(); ++i) {
      json image;

      UpdateImageObject(model->images[i], baseDir, int(i), embedImages,
                        &this->WriteImageData, this->write_image_user_data_);
      SerializeGltfImage(model->images[i], image);
      images.push_back(image);
    }
    output["images"] = images;
  }

  // MATERIALS
  if (model->materials.size()) {
    json materials;
    for (unsigned int i = 0; i < model->materials.size(); ++i) {
      json material;
      SerializeGltfMaterial(model->materials[i], material);
      materials.push_back(material);
    }
    output["materials"] = materials;
  }

  // MESHES
  if (model->meshes.size()) {
    json meshes;
    for (unsigned int i = 0; i < model->meshes.size(); ++i) {
      json mesh;
      SerializeGltfMesh(model->meshes[i], mesh);
      meshes.push_back(mesh);
    }
    output["meshes"] = meshes;
  }

  // NODES
  if (model->nodes.size()) {
    json nodes;
    for (unsigned int i = 0; i < model->nodes.size(); ++i) {
      json node;
      SerializeGltfNode(model->nodes[i], node);
      nodes.push_back(node);
    }
    output["nodes"] = nodes;
  }

  // SCENE
  if (model->defaultScene > -1) {
    SerializeNumberProperty<int>("scene", model->defaultScene, output);
  }

  // SCENES
  if (model->scenes.size()) {
    json scenes;
    for (unsigned int i = 0; i < model->scenes.size(); ++i) {
      json currentScene;
      SerializeGltfScene(model->scenes[i], currentScene);
      scenes.push_back(currentScene);
    }
    output["scenes"] = scenes;
  }

  // SKINS
  if (model->skins.size()) {
    json skins;
    for (unsigned int i = 0; i < model->skins.size(); ++i) {
      json skin;
      SerializeGltfSkin(model->skins[i], skin);
      skins.push_back(skin);
    }
    output["skins"] = skins;
  }

  // TEXTURES
  if (model->textures.size()) {
    json textures;
    for (unsigned int i = 0; i < model->textures.size(); ++i) {
      json texture;
      SerializeGltfTexture(model->textures[i], texture);
      textures.push_back(texture);
    }
    output["textures"] = textures;
  }

  // SAMPLERS
  if (model->samplers.size()) {
    json samplers;
    for (unsigned int i = 0; i < model->samplers.size(); ++i) {
      json sampler;
      SerializeGltfSampler(model->samplers[i], sampler);
      samplers.push_back(sampler);
    }
    output["samplers"] = samplers;
  }

  // CAMERAS
  if (model->cameras.size()) {
    json cameras;
    for (unsigned int i = 0; i < model->cameras.size(); ++i) {
      json camera;
      SerializeGltfCamera(model->cameras[i], camera);
      cameras.push_back(camera);
    }
    output["cameras"] = cameras;
  }

  // EXTENSIONS
  SerializeExtensionMap(model->extensions, output);

  // LIGHTS as KHR_lights_cmn
  if (model->lights.size()) {
    json lights;
    for (unsigned int i = 0; i < model->lights.size(); ++i) {
      json light;
      SerializeGltfLight(model->lights[i], light);
      lights.push_back(light);
    }
    json khr_lights_cmn;
    khr_lights_cmn["lights"] = lights;
    json ext_j;

    if (output.find("extensions") != output.end()) {
      ext_j = output["extensions"];
    }

    ext_j["KHR_lights_cmn"] = khr_lights_cmn;

    output["extensions"] = ext_j;
  }

  // EXTRAS
  if (model->extras.Type() != NULL_TYPE) {
    SerializeValue("extras", model->extras, output);
  }

  if (writeBinary) {
    WriteBinaryGltfFile(filename, output.dump());
  } else {
    WriteGltfFile(filename, output.dump(prettyPrint ? 2 : -1));
  }

  return true;
}

}  // namespace tinygltf

#ifdef __clang__
#pragma clang diagnostic pop
#endif