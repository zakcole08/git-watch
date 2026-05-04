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

	// create git-watch branch if it doesnt exist
	git_reference *branch_ref = NULL;
	if (git_branch_lookup(&branch_ref, repo, "git-watch", GIT_BRANCH_LOCAL) != 0) {
		// create from current HEAD
		git_oid head_oid;
		git_commit *head_commit = NULL;

		if (git_reference_name_to_id(&head_oid, repo, "HEAD") == 0 &&
				git_commit_lookup(&head_commit, repo, &head_oid) == 0) {

			git_branch_create(&branch_ref, repo, "git-watch", head_commit, 0);
			git_commit_free(head_commit);
		}
	}

	//switch HEAD to git-watch
	git_repository_set_head(repo, "refs/heads/git-watch");

	git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
	opts.checkout_strategy = GIT_CHECKOUT_SAFE;
	git_checkout_head(repo, &opts);

	git_index *index = NULL;
	git_repository_index(&index, repo);

	git_index_add_all(
			index,
			NULL,
			GIT_INDEX_ADD_DEFAULT,
			add_callback,
			repo
			);

	git_index_write(index);

	git_oid tree_oid, commit_oid;
	git_tree *tree = NULL;

	git_index_write_tree(&tree_oid, index);
	git_tree_lookup(&tree, repo, &tree_oid);

	git_diff *diff = NULL;

	git_diff_index_to_workdir(&diff, repo, NULL, NULL);

	if (git_diff_num_deltas(diff) == 0) {
		printf("Nothing to commit\n");
		git_diff_free(diff);
		return 0;
	}

	git_diff_free(diff);

	git_index_free(index);


	git_signature *sig = NULL;

	if (git_signature_default(&sig, repo) != 0) {
		printf("Git user.name or user.email not set\n");
		printf("Run: git config --global user.email \"you@example.com\"\ngit config --global user.name \"your-username\"\n");
		return 1;
	}

	git_commit *parent = NULL;
	git_oid parent_oid;

	if (git_reference_name_to_id(&parent_oid, repo, "HEAD") == 0) {
		git_commit_lookup(&parent, repo, &parent_oid);
	}

	if (git_commit_create(
				&commit_oid,
				repo,
				"HEAD",
				sig,
				sig,
				NULL,
				"auto commit by git-watch",
				tree,
				parent ? 1 : 0,
				parent ? (const git_commit **)&parent : NULL
				) == 0) {

		char oid_str[GIT_OID_HEXSZ + 1];
		git_oid_tostr(oid_str, sizeof(oid_str), &commit_oid);

		printf("\n=== Commit Created ===\n");
		printf("Branch: git-watch\n");
		printf("Commit: %s\n", oid_str);
	} else {
		printf("Failed to create commit\n");
	}

	// cleanup
	git_tree_free(tree);
	git_signature_free(sig);
	if (parent) git_commit_free(parent);
	if (branch_ref) git_reference_free(branch_ref);

	git_repository_free(repo);
	git_libgit2_shutdown();

	return 0;
}
