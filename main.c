#include <git2.h>
#include <stdio.h>

int print_usage() {
	printf("Usage: ./git-watch /path/to/repo\n");
	return 0;
}

int stage_file(git_repository *repo, const char *path) {
	git_index *index = NULL;
	int error = 0;

	// Get the repository index
	if ((error = git_repository_index(&index, repo)) < 0)
		return error;

	// Add the file path to the index (stage it)
	if ((error = git_index_add_bypath(index, path)) < 0) {
		git_index_free(index);
		return error;
	}

	// Write the updated index back to disk
	if ((error = git_index_write(index)) < 0) {
		git_index_free(index);
		return error;
	}

	git_index_free(index);
	return 0;
}

int stage_file_loop(git_repository *repo, const char *path) {
	printf("Untracked: %s\n", path);
	if (stage_file(repo, path) != 0) {
		printf("Failed to stage file %s\n", path);
	}
	else {
		printf("Successfully staged file %s\n", path);
	}
}

int main(int argc, char **argv) {
	if (argc < 2) {
		print_usage();
		return 1;
	}

	git_libgit2_init();
	git_repository *repo = NULL;
	git_status_list *status = NULL;
	git_status_options opts = GIT_STATUS_OPTIONS_INIT;

	// Open the repository in the specified directory
	if (git_repository_open(&repo, argv[1]) != 0) {
		printf("Could not open repo at %s\n", argv[1]);
		return 1;
	}

	// Configure options to include untracked files
	opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
	opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS;

	git_status_list_new(&status, repo, &opts);
	size_t count = git_status_list_entrycount(status);

	for (size_t i = 0; i < count; i++) {
		const git_status_entry *s = git_status_byindex(status, i);
		// Check if the file status is "Worktree New" (untracked)
		if (s->status & GIT_STATUS_WT_NEW) {
			const char *path = s->index_to_workdir->new_file.path;
			stage_file_loop(repo, path);
		}
		else {
			printf("No untracked files found in %s", argv[1]);
		}
	}

		git_status_list_free(status);
		git_repository_free(repo);
		git_libgit2_shutdown();
		return 0;
	}

