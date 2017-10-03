#define DG_FILEHANDLING_IMPLEMENTATION
#include "../DG_filehandling.h"

#include <stdio.h>

#define eprintf(...) fprintf(stderr, __VA_ARGS__)

int main(int argc, char** argv)
{
	if(argc < 2)
	{
		eprintf("Usage: %s <path-to-list>\n", argv[0]);
		return 1;
	}
	
	const char* path = argv[1];
	
	dgfh_dir* dir = dgfh_opendir(path, DGFH_ALL);
	if(dir == NULL)
	{
		eprintf("dgfh_opendir() returned NULL!\n");
		return 1;
	}
	
	printf("%s contains:\n", path);
	for(dgfh_dirent* ent = dgfh_next_dir_entry(dir); ent != NULL; ent = dgfh_next_dir_entry(dir))
	{
		printf("  %s type %d\n", ent->name, ent->type);
	}
	
	dgfh_close_dir(dir);
	dir = NULL;
	
	return 0;
}
