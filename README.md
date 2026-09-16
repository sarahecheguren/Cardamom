# Cardamom

School project built with C++ and the [Drogon](https://drogon.org) web framework, backed by MariaDB, with an HTML/CSS/JavaScript front end.

## Getting started

```bash
git clone https://github.com/sarahecheguren/Cardamom.git
cd Cardamom
```

Open the folder in VS Code and accept the prompt to install the recommended extensions. That gives you format-on-save for every file type in the project.

## Code formatting

Two formatters are used, scoped by file type. Both configs live at the repo root, so you never need to choose one manually.

| Files                    | Tool         | Config            |
| ------------------------ | ------------ | ----------------- |
| `.cc` `.cpp` `.h` `.hpp` | clang-format | `.clang-format`   |
| `.html` `.css` `.js`     | Prettier     | `.prettierrc`     |
| `.csp` Drogon views      | none         | `.prettierignore` |

### Install the tools

- **clang-format**: `brew install clang-format` on macOS, `sudo apt install clang-format` on Ubuntu, or it ships with LLVM on Windows.
- **Prettier**: needs Node.js. No install required; it runs through `npx`.

### Format before you push

```bash
# C++
clang-format -i $(git ls-files '*.cc' '*.cpp' '*.h' '*.hpp')

# HTML, CSS, JavaScript
npx prettier@3 --write .
```

Pull requests run both checks automatically. A PR with unformatted code cannot be merged, so run the commands above if the check fails.

### SQL

There is no SQL formatter. Keep keywords uppercase and one clause per line in schema and migration files.

## Contributing

1. Create a branch from `main`: `git checkout -b feature/your-change`
2. Commit your work and push the branch.
3. Open a pull request into `main`. Direct pushes to `main` are blocked.
4. Wait for the format checks to pass and get one approval, then merge.
