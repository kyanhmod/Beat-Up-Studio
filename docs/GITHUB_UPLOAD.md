# Upload the prepared source

The prepared folder and ZIP contain src/, tests/, tools/, docs/, build.bat,
README and notices, .gitignore/.gitattributes and assets/default.pack.
They omit EXEs, compiler objects, test runtime files, music, client binaries,
reference checkouts, raw texture directories and alternative skins.

Create an empty GitHub repository, then initialize Git in this prepared folder,
add and commit its contents, and add the repository URL as origin. Push the main
branch using your normal GitHub authentication. No repository is created or
published by the preparation script, and no credentials are stored here.

The complete EXE belongs in a GitHub Release rather than a source commit.
