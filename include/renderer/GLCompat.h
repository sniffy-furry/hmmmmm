#pragma once

// Single include point for renderer/OpenGL users.
// Desktop builds use glad as before; Android builds use the NDK GLES headers.
#if defined(__ANDROID__)
#  include <GLES3/gl3.h>
#  include <GLES3/gl3ext.h>
#  if !defined(GL_BGRA) && defined(GL_BGRA_EXT)
#    define GL_BGRA GL_BGRA_EXT
#  endif
#  if !defined(GL_POLYGON_OFFSET_LINE)
#    define GL_POLYGON_OFFSET_LINE GL_POLYGON_OFFSET_FILL
#  endif
#else
#  include <glad/glad.h>
#endif
