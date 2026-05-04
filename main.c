#include <git2.h>
#include <stdio.h>

int add_callback(const char *path, const char *matched_pathspec, void *payload) {
	git_repository *repo = (git_repository *)payload;
	const char *workdir = git_repository_workdir(repo);
	printf("Adding: %s%s\n", workdir, path);
	return 0;
}

int main(int argc, char **argv) {
	if (argc < 2) {
		printf("Usage: ./git-watch /path/to/repo\n");
		return 1;
	}

	git_libgit2_init();

	git_repository *repo = NULL;
	if (git_repository_open(&repo, argv[1]) != 0) {
		printf("Could not open repo\n");
		return 1;
	}

	git_index *index = NULL;
	git_repository_index(&index, repo);

	git_index_add_all(
			index,
			NULL, // all files
			GIT_INDEX_ADD_DEFAULT,
			add_callback,
			repo   // pass repo into callback
			);

	git_index_write(index);

	git_index_free(index);
	git_repository_free(repo);
	git_libgit2_shutdown();

	return 0;
}
