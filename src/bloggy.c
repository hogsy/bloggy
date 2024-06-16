// SPDX-License-Identifier: MIT
// bloggy - simple tool, simple blog
// Copyright © 2024 Mark E Sowden <hogsy@oldtimes-software.com>

#define _XOPEN_SOURCE// needed for strptime
#include <time.h>

#include "bloggy.h"

#define MAX_SOCIALS 16

typedef struct Social
{
	char name[ 64 ];
	char url[ 256 ];
} Social;

struct Config
{
	char title[ 64 ];
	char subtitle[ 64 ];
	char author[ 64 ];

	Social socials[ MAX_SOCIALS ];
	unsigned int numSocials;
} config = {};

static PLVectorArray *blogPosts;

static time_t str_to_timestamp( const char *string )
{
	if ( strlen( string ) <= 10 )
	{
		printf( "Cannot convert string (%s) to timestamp, invalid length!\n", string );
		return 0;
	}

	struct tm tm = {};
	strptime( string, "%Y-%m-%d", &tm );
	time_t timestamp = mktime( &tm );
	if ( timestamp == -1 )
	{
		printf( "Failed to convert date string to timestamp!\n" );
		return 0;
	}

	return timestamp;
}

static char *load_to_buf( const char *path )
{
	PLFile *file = PlOpenFile( path, true );
	if ( file == NULL )
	{
		printf( "Failed to open file: %s\n", PlGetError() );
		return NULL;
	}

	size_t fileSize = PlGetFileSize( file );
	char *buf = PL_NEW_( char, fileSize + 1 );
	size_t bytesRead = PlReadFile( file, buf, sizeof( char ), fileSize );
	if ( bytesRead != fileSize )
	{
		printf( "Failed to read file: %s\n", PlGetError() );
		PL_DELETEN( buf );
	}

	PlCloseFile( file );
	return buf;
}

static Post *parse_post_buf( const char *buf )
{
	Post *post = PL_NEW( Post );

	const char *p = buf;
	while ( *p != '\0' )
	{
		char token[ 64 ];
		PlParseToken( &p, token, sizeof( token ) );
		if ( *token == '\0' )
		{
			PlSkipLine( &p );
			continue;
		}
		else if ( strcmp( token, "title" ) == 0 )
		{
			PlParseEnclosedString( &p, post->title, sizeof( post->title ) );
			PlSkipLine( &p );
			continue;
		}
		else if ( strcmp( token, "----" ) == 0 )
		{
			PlSkipLine( &p );

			// this is where the main body of the post begins - let's determine how big it is first
			size_t length = strlen( p );
			post->body = PL_NEW_( char, length + 1 );
			for ( unsigned int i = 0; i < length; ++i )
			{
				post->body[ i ] = *( p++ );
			}

			break;
		}

		printf( "Unknown token (%s), skipping!\n", token );
		PlSkipLine( &p );
	}

	return post;
}

static void index_post( const char *path, void * )
{
	// convert the filename into a timestamp for sorting later
	const char *filename = PlGetFileName( path );
	time_t timestamp = str_to_timestamp( filename );
	if ( timestamp == 0 )
	{
		return;
	}

	// now parse the file itself
	char *buf = load_to_buf( path );
	if ( buf == NULL )
	{
		return;
	}

	parse_post_buf( buf );

	Post *in = parse_post_buf( buf );
	if ( in != NULL )
	{
		strncpy( in->id, filename, 10 );
		in->timestamp = timestamp;

		PlPushBackVectorArrayElement( blogPosts, in );

		printf( "Indexed post, %s (%s)\n", in->id, *in->title != '\0' ? in->title : "n/a" );
	}

	PL_DELETE( buf );
}

static int compare_timestamps( const void *p1, const void *p2 )
{
	const Post *a = *( Post ** ) p1;
	const Post *b = *( Post ** ) p2;
	if ( a->timestamp > b->timestamp )
	{
		return -1;
	}
	else if ( a->timestamp < b->timestamp )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

static void print_html_header( FILE *file )
{
	fprintf( file, "<!DOCTYPE html>\n"
	               "<html lang=\"en-US\">"
	               "<!-- Page generated by bloggy -->" );

	// head section
	fprintf( file,
	         "<head>"
	         "<meta charset=\"UTF-8\">"
	         "<link rel=\"icon\" type=\"image/png\" href=\"favicon-32x32.png\" sizes=\"32x32\">"
	         "<link rel=\"icon\" type=\"image/png\" href=\"favicon-16x16.png\" sizes=\"16x16\">"
	         "<link rel=\"stylesheet\" href=\"style.css?ver=%lu\">"
	         "<title>%s</title>"
	         "</head>",
	         time( NULL ), config.title );
}

static void print_html_footer( FILE *file )
{
	fprintf( file, "<div class=\"footer\">" );
	{
		// going to do something cheeky here and determine the start and end for copyright range based on the posts

		char copyPeriod[ 16 ] = {};
		Post *firstPost = PlGetVectorArrayBack( blogPosts );
		Post *lastPost = PlGetVectorArrayFront( blogPosts );
		assert( firstPost != NULL && lastPost != NULL );
		if ( firstPost == lastPost )
		{
			strncpy( copyPeriod, firstPost->id, 4 );
		}
		else
		{
			char startYear[ 5 ] = {};
			strncpy( startYear, ( ( Post * ) PlGetVectorArrayBack( blogPosts ) )->id, 4 );
			char endYear[ 5 ] = {};
			strncpy( endYear, ( ( Post * ) PlGetVectorArrayFront( blogPosts ) )->id, 4 );
			snprintf( copyPeriod, sizeof( copyPeriod ), "%s-%s", startYear, endYear );
		}

		fprintf( file, "<p>" );
		for ( unsigned int i = 0; i < config.numSocials; ++i )
		{
			fprintf( file, "<a rel=\"me\" href=\"%s\">%s</a>", config.socials[ i ].url, config.socials[ i ].name );
			fprintf( file, i < ( config.numSocials - 1 ) ? " &#8285; " : "<br>" );
		}

		fprintf( file, "Copyright &copy; %s %s<br>", copyPeriod, config.author );
		fprintf( file, "Generated by <a href=\"https://github.com/hogsy/bloggy\">bloggy</a> (%s).</p>", PlGetFormattedTime() );
	}
	fprintf( file, "</div>" );
}

static void write_html_homepage( void )
{
	FILE *file = fopen( "web/index.htm", "w" );

	print_html_header( file );

	fprintf( file, "<div class=\"header\">" );
	fprintf( file, "<h1 class=\"title\">%s</h1>", config.title );
	if ( *config.subtitle != '\0' )
	{
		fprintf( file, "<h1 class=\"subtitle\">%s</h1>", config.subtitle );
	}
	fprintf( file, "<hr></div>" );

	unsigned int numPosts;
	Post **posts = ( Post ** ) PlGetVectorArrayDataEx( blogPosts, &numPosts );
	for ( unsigned int i = 0; i < numPosts; ++i )
	{
		fprintf( file, "<div class=\"blog-item\"><a href=\"%s.htm\">%s", posts[ i ]->id, posts[ i ]->id );
		if ( *posts[ i ]->title != '\0' )
		{
			fprintf( file, " - %s", posts[ i ]->title );
		}
		fprintf( file, "</a></div>" );
	}

	print_html_footer( file );

	fclose( file );
}

static void write_html_pages( void )
{
	if ( !PlCreateDirectory( "web" ) )
	{
		printf( "Failed to create output directory: %s\n", PlGetError() );
		return;
	}

	PlCopyFile( "style.css", "web/style.css" );

	write_html_homepage();

	unsigned int numPosts;
	Post **posts = ( Post ** ) PlGetVectorArrayDataEx( blogPosts, &numPosts );
	for ( unsigned int i = 0; i < numPosts; ++i )
	{
		PLPath path;
		PlSetupPath( path, true, "web/%s.htm", posts[ i ]->id );

		FILE *file = fopen( path, "w" );
		if ( file == NULL )
		{
			printf( "Failed to write post (%s)!\n", path );
			continue;
		}

		print_html_header( file );

		fprintf( file, "<a href=\"index.htm\">&larr; Back</a>" );

		fprintf( file, "<div class=\"post-header\">" );
		if ( *posts[ i ]->title != '\0' )
		{
			fprintf( file, "<h1 class=\"post-title\">%s</h1>", posts[ i ]->title );
		}
		fprintf( file, "<h1 class=\"post-subtitle\">%s / %s</h2>", posts[ i ]->id, config.author );
		fprintf( file, "<hr></div>" );

		fprintf( file, "<div class=\"post-body\">" );
		if ( posts[ i ]->body != NULL )
		{
			fprintf( file, "%s", posts[ i ]->body );
			PL_DELETEN( posts[ i ]->body );
		}
		fprintf( file, "</div>" );

		print_html_footer( file );

		fclose( file );
	}
}

static bool load_config( void )
{
	char *buf = load_to_buf( "bloggy.conf" );
	if ( buf == NULL )
	{
		return false;
	}

	const char *p = buf;
	while ( *p != '\0' )
	{
		char token[ 64 ];
		PlParseToken( &p, token, sizeof( token ) );
		if ( *token == '\0' )
		{
			PlSkipLine( &p );
			continue;
		}
		else if ( strcmp( token, "title" ) == 0 )
		{
			PlParseEnclosedString( &p, config.title, sizeof( config.title ) );
			PlSkipLine( &p );
			continue;
		}
		else if ( strcmp( token, "subtitle" ) == 0 )
		{
			PlParseEnclosedString( &p, config.subtitle, sizeof( config.subtitle ) );
			PlSkipLine( &p );
			continue;
		}
		else if ( strcmp( token, "author" ) == 0 )
		{
			PlParseEnclosedString( &p, config.author, sizeof( config.author ) );
			PlSkipLine( &p );
			continue;
		}
		else if ( strcmp( token, "social" ) == 0 )
		{
			if ( config.numSocials >= MAX_SOCIALS )
			{
				printf( "Hit maximum socials limit (%u >= %u)!", config.numSocials, MAX_SOCIALS );
				PlSkipLine( &p );
				continue;
			}

			Social *social = &config.socials[ config.numSocials ];
			PlParseEnclosedString( &p, social->name, sizeof( social->name ) );
			PlParseEnclosedString( &p, social->url, sizeof( social->url ) );
			config.numSocials++;
			PlSkipLine( &p );
			continue;
		}
		printf( "Unknown token (%s), skipping!\n", token );
		PlSkipLine( &p );
	}

	PL_DELETE( buf );

	return true;
}

static void copy_asset( const char *path, void *user )
{
	PLPath dest;
	PlSetupPath( dest, true, "web/%s", path + strlen( ( const char * ) user ) );

	// need to create a destination path if it's under a subdir
	PLPath temp;
	strcpy( temp, dest );
	char *c = strrchr( temp, '/' );
	*c = '\0';
	if ( !PlCreatePath( temp ) )
	{
		printf( "Failed to create path (%s): %s\n", temp, PlGetError() );
		return;
	}

	if ( PlCopyFile( path, dest ) )
	{
		printf( "Copied \"%s\" to \"%s\"\n", path, dest );
		return;
	}

	printf( "Failed to copy asset (%s) to destination (%s): %s\n", path, dest, PlGetError() );
}

int main( int argc, char **argv )
{
	printf( "bloggy - simple tool, simple blog\n"
	        "Created by Mark E. Sowden\n"
	        "---------------------------------\n" );

	if ( PlInitialize( argc, argv ) != PL_RESULT_SUCCESS )
	{
		printf( "Failed to initialize Hei library: %s\n", PlGetError() );
		return EXIT_FAILURE;
	}

	double startTime = PlGetCurrentSeconds();

	if ( !load_config() )
	{
		return EXIT_FAILURE;
	}

	blogPosts = PlCreateVectorArray( 32 );

	PlScanDirectory( "assets/", NULL, copy_asset, true, "assets/" );
	PlScanDirectory( "posts/", "post", index_post, false, blogPosts );

	// now qsort all the blog posts based on their timestamp
	unsigned int numPosts;
	void **array = PlGetVectorArrayDataEx( blogPosts, &numPosts );
	qsort( array, numPosts, sizeof( Post * ), compare_timestamps );

	write_html_pages();

	PlShutdown();

	double endTime = PlGetCurrentSeconds();

	printf( "Site generated in %f seconds.\n", endTime - startTime );

	return EXIT_SUCCESS;
}
