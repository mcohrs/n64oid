/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-ui-console - osal_preproc.h                               *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2009 Richard Goedeken                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* This header file is for OS-specific #includes and #defines
 *
 */

#if !defined(OSAL_PREPROC_H)
#define OSAL_PREPROC_H

#if defined(WIN32)

  #include <windows.h>
  #define PATH_MAX 2048
  #define OSAL_DEFAULT_DYNLIB_FILENAME "mupen64plus.dll"
  #define OSAL_DIR_SEPARATOR           '\\'
  #define OSAL_CURRENT_DIR             ".\\"
  #define OSAL_DLL_EXTENSION           ".dll"

#elif defined(__APPLE__)

  #include <limits.h>  // for PATH_MAX
  #define OSAL_DEFAULT_DYNLIB_FILENAME "libmupen64plus.dylib"
  #define OSAL_DIR_SEPARATOR           '/'
  #define OSAL_CURRENT_DIR             "./"
  #define OSAL_DLL_EXTENSION           ".dylib"
  
#else  /* Linux-like UNIX */

  #include <limits.h>  // for PATH_MAX
  #define OSAL_DEFAULT_DYNLIB_FILENAME "libn64oidcore.so"
  #define OSAL_DIR_SEPARATOR           '/'
  //#define OSAL_CURRENT_DIR             "./"
  #define OSAL_CURRENT_DIR             "/data/local/n64oid/"
  #define OSAL_DLL_EXTENSION           ".so"

#endif

#endif /* #define OSAL_PREPROC_H */

