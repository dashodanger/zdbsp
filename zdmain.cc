/*
    The main glue for ZDBSP.
    Copyright (C) 2002-2006 Randy Heit

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

// HEADER FILES ------------------------------------------------------------

#ifdef _WIN32

// Need windows.h for QueryPerformanceCounter
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define HAVE_TIMING 1
#define START_COUNTER(s,e,f) \
	LARGE_INTEGER s, e, f; QueryPerformanceCounter (&s);
#define END_COUNTER(s,e,f,l) \
	QueryPerformanceCounter (&e); QueryPerformanceFrequency (&f); \
	if (!NoTiming) printf (l, double(e.QuadPart - s.QuadPart) / double(f.QuadPart));

#else

#include <time.h>
#define HAVE_TIMING 1
#define START_COUNTER(s,e,f) \
	clock_t s, e; s = clock();
#define END_COUNTER(s,e,f,l) \
	e = clock(); \
	if (!NoTiming) printf (l, double(e - s) / CLOCKS_PER_SEC);

// Need these to check if input/output are the same file
//#include <sys/types.h>
//#include <sys/stat.h>

#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "zdmain.h"
#include "zdwad.h"
#include "processor.h"

// MACROS ------------------------------------------------------------------

#ifndef M_PI
#define M_PI            3.14159265358979323846
#endif

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static void ShowVersion ();

// PUBLIC DATA DEFINITIONS -------------------------------------------------

const char		*Map = NULL;
const char		*InName;
const char		*OutName = "tmp.wad";
bool			 BuildNodes = true;
bool			 BuildGLNodes = false;
bool			 ConformNodes = false;
bool			 NoPrune = false;
EBlockmapMode	 BlockmapMode = EBM_Rebuild;
ERejectMode		 RejectMode = ERM_DontTouch;
bool			 WriteComments = false;
int				 MaxSegs = 64;
int				 SplitCost = 8;
int				 AAPreference = 16;
bool			 CheckPolyobjs = true;
bool			 ShowMap = false;
bool			 ShowWarnings = true;
bool			 NoTiming = false;
bool			 CompressNodes = true;
bool			 CompressGLNodes = true;
bool			 ForceCompression = false;
bool			 GLOnly = false;
bool			 V5GLNodes = false;

// CODE --------------------------------------------------------------------

int zdmain (const char *filename, zdbsp_options options)
{

	BuildNodes = options.build_nodes;
	BuildGLNodes = options.build_gl_nodes;
	RejectMode = options.reject_mode;
	CheckPolyobjs = options.check_polyobjs;
	CompressNodes = options.compress_nodes;
	CompressGLNodes = options.compress_gl_nodes;
	ForceCompression = options.force_compression;
	GLOnly = options.build_gl_only;

    ShowVersion();

    bool fixSame = false;

    InName = filename;

	try
	{
		START_COUNTER(t1a, t1b, t1c)

		if (true)
		{
			// When the input and output files are the same, output will go to
			// a temporary file. After everything is done, the input file is
			// deleted and the output file is renamed to match the input file.

			char *out = new char[strlen(OutName)+3], *dot;

			if (out == NULL)
			{
				throw std::runtime_error("Could not create temporary file name.");
			}

			strcpy (out, OutName);
			dot = strrchr (out, '.');
			if (dot && (dot[1] == 'w' || dot[1] == 'W')
					&& (dot[2] == 'a' || dot[2] == 'A')
					&& (dot[3] == 'd' || dot[3] == 'D')
					&& dot[4] == 0)
			{
				// *.wad becomes *.daw
				dot[1] = 'd';
				dot[3] = 'w';
			}
			else
			{
				// * becomes *.x
				strcat (out, ".x");
			}
			OutName = out;
			fixSame = true;
		}

		{
			FWadReader inwad (InName);
			FWadWriter outwad (OutName, inwad.IsIWAD());

			int lump = 0;
			int max = inwad.NumLumps ();

			while (lump < max)
			{
				if (inwad.IsMap (lump) &&
					(!Map || strcasecmp(inwad.LumpName (lump), Map) == 0))
				{
					START_COUNTER(t2a, t2b, t2c)
					FProcessor builder (inwad, lump);
					builder.Write (outwad);
					END_COUNTER(t2a, t2b, t2c, "   %.3f seconds.\n")

					lump = inwad.LumpAfterMap (lump);
				}
				else if (inwad.IsGLNodes (lump))
				{
					// Ignore GL nodes from the input for any maps we process.
					if (BuildNodes && (Map == NULL || strcasecmp(inwad.LumpName (lump)+3, Map) == 0))
					{
						lump = inwad.SkipGLNodes (lump);
					}
					else
					{
						outwad.CopyLump (inwad, lump);
						++lump;
					}
				}
				else
				{
					//printf ("copy %s\n", inwad.LumpName (lump));
					outwad.CopyLump (inwad, lump);
					++lump;
				}
			}

			outwad.Close ();
		}

		if (fixSame)
		{
			remove (InName);
			if (0 != rename (OutName, InName))
			{
				printf ("The output file could not be renamed to %s.\nYou can find it as %s.\n",
					InName, OutName);
			}
		}

		END_COUNTER(t1a, t1b, t1c, "\nTotal time: %.3f seconds.\n")
	}
	catch (std::runtime_error &msg)
	{
		printf ("%s\n", msg.what());
		return 20;
	}
	catch (std::bad_alloc &msg)
	{
		printf ("Out of memory\n");
		return 20;
	}
	catch (std::exception &msg)
	{
		printf ("%s\n", msg.what());
		return 20;
	}
#ifndef _DEBUG
	catch (...)
	{
		printf ("Unhandled exception. ZDBSP cannot continue.\n");
		return 20;
	}
#endif

	return 0;
}

//==========================================================================
//
// ShowVersion
//
//==========================================================================

static void ShowVersion ()
{
	printf ("ZDBSP " ZDBSP_VERSION " ("
#if defined(__GNUC__)

		"GCC"
#if defined(__i386__)
		"-x86"
#elif defined(__amd64__)
		"-amd64"
#elif defined(__ppc__)
		"-ppc"
#endif

#elif defined(_MSC_VER)

		"VC"
#if defined(_M_IX86)
		"-x86"
#if _M_IX86_FP > 1
		"-SSE2"
#endif
#elif defined(_M_X64)
		"-x64"
#endif

#endif

		" : " __DATE__ ")\n");
}

//==========================================================================
//
// PointToAngle
//
//==========================================================================

angle_t PointToAngle (fixed_t x, fixed_t y)
{
	double ang = atan2 (double(y), double(x));
	const double rad2bam = double(1<<30) / M_PI;
	double dbam = ang * rad2bam;
	// Convert to signed first since negative double to unsigned is undefined.
	return angle_t(int(dbam)) << 1;
}

//==========================================================================
//
// Warn
//
//==========================================================================

void Warn (const char *format, ...)
{
	va_list marker;

	if (!ShowWarnings)
	{
		return;
	}

	va_start (marker, format);
	vprintf (format, marker);
	va_end (marker);
}
