#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "location.h"
#include "config.h"

struct location_type *type_start = NULL;

int location_init( void ) {
	struct config_location_type *config_type = config_get()->location_types;
	
	while( config_type ) {
		struct location_type *type = malloc( sizeof(struct location_type) );
		
		if( type ) {
			struct config_location *config_location = config_get()->locations;
			
			type->locations = NULL;
			type->name = config_type->name;
			
			while( config_location ) {
				if( config_location->type == config_type ) {
					struct location *location = malloc( sizeof(struct location) );
					if( location ) {
						location->directory = config_location->directory;
						location->next = type->locations;
						type->locations = location;
					}
					else {
						fprintf( stderr, "Warning: Couldn't allocate memory for location\n" );
					}
				}
				config_location = config_location->next;
			}
			type->next = type_start;
			type_start = type;
		}
		else {
			fprintf( stderr, "Warning: Couldn't allocate memory for location type\n" );
		}
		config_type = config_type->next;
	}

/*{
	struct location_type *type = type_start;
	while( type ) {
		struct location *location = type->locations;
		printf("Type: %s\n", type->name );
		while( location ) {
			printf("  %s\n", location->directory );
			location = location->next;
		}
		type = type->next;
	}
}*/

	return 0;	
}

void location_free( void ) {
	struct location_type *type = type_start;
	
	while( type ) {
		struct location_type *ttmp = type->next;
		struct location *location = type->locations;
		
		while( location ) {
			struct location *ltmp = location->next;
			free( location );
			location = ltmp;
		}
		
		free( type );
		type = ttmp;
	}
}

struct location *location_get_first( const char *type ) {
	struct location_type *ltype = type_start;
	
	while( ltype ) {
		if( strcasecmp( ltype->name, type ) == 0 )
			return ltype->locations;
		ltype = ltype->next;
	}
	
	return NULL;
}

int location_get_path( const char *type, const char *filename, char *path ) {
	struct location *location = location_get_first( type );
	char test[CONFIG_FILE_NAME_LENGTH];

	if( type && filename ) {
#ifdef __WIN32__
		if( filename[0] && filename[1] == ':' ) {
#else
		if( filename[0] == '/' ) {
#endif
			/* File name is already absolute - return the original value */
			strncpy( path, filename, CONFIG_FILE_NAME_LENGTH );
			return 0;
		}
		
		while( location ) {
#ifdef __WIN32__
			snprintf( test, CONFIG_FILE_NAME_LENGTH, "%s\\%s", location->directory, filename );
#else
			snprintf( test, CONFIG_FILE_NAME_LENGTH, "%s/%s", location->directory, filename );
#endif
			if( open( test, O_RDONLY ) == -1 ) {
				if( errno != ENOENT ) {
					fprintf( stderr, "Warning: Couldn't read file '%s': %s", test, strerror( errno ) );
					break;
				}
			}
			else {
				strcpy( path, test );
				return 0;
			}
		}
	}

	fprintf( stderr, "Warning: Couldn't find file '%s' in any location with type '%s'\n", filename, type );
	path = NULL;
	return -1;
}

