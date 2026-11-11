#ifndef FRONTMOVIE_H
#define FRONTMOVIE_H

#include "xport.h"
void InitializeFrontMovie(sint32 &session);
void StopFrontMovie(sint32 &session);
bool FrontMovieNameMatches(const char *movieName, const char *expectedName);
bool StartFrontMovie(sint32 &session, const char *logicalPath, const char *preparedPath);
sint32 UploadFrontMovieFrame(sint32 &session, const sint32 &frameCounter);
sint32 UploadFrontMovieFrameSized(sint32 &session, const sint32 &frameCounter, sint32 width, sint32 height);

#endif
