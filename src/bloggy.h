// SPDX-License-Identifier: MIT
// bloggy - simple tool, simple blog
// Copyright © 2024 Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "plcore/pl.h"
#include "plcore/pl_filesystem.h"
#include "plcore/pl_parse.h"
#include "plcore/pl_array_vector.h"
#include "plcore/pl_timer.h"

typedef struct Post
{
	time_t timestamp;
	char id[ 11 ];   // essentially the date, i.e., 2000-00-00
	char title[ 64 ];// optional field
	char *body;      // bulk html body of the post
} Post;
