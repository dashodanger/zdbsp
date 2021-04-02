# zdbsp
ZDBSP is ZDoom's internal node builder.

This repo has the altered version of ZDBSP that I used for Obsidian's internal node builder. 
Instead of being a standalone program, the main function now accepts a singular filename and a struct containing
various options (check zdmain.h).

I implemented the existing, but unsused reject builders for both non-GL (DoomBSP algorithm) and GL nodes (Vis adaptation).

I've removed various references to SSE/SSE2 to facilitate cross-platform compilation.
