#ifndef MOVIE_H
#define MOVIE_H

#include "xport.h"
sint32 play_movie(char movie);
extern sint16 user_exit;
extern char skip_all;
extern char gPlayerNum;

class Movie
{
public:
    static sint32 Play(char movie);
};

#endif
