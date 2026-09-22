#!/usr/bin/env bash

set -euo pipefail

usage()
{
    printf 'Usage: %s [PROJECT NAME]\n' "$0"
}

if [[ $# -eq 1 && ( $1 == --help || $1 == -h ) ]]; then
    usage
    exit 0
fi

if [[ $# -ne 1 ]]; then
    usage >&2
    exit 1
fi

project_name=$1

if [[ ! $project_name =~ ^[a-zA-Z0-9][a-zA-Z0-9_-]*$ ]]; then
    printf 'Error: Project names must start with a letter or digit and contain only letters, digits, underscores, or hyphens.\n' >&2
    exit 1
fi

project_dir="./$project_name"

if [[ -e $project_dir || -L $project_dir ]]; then
    printf 'Error: Path already exists: %s\n' "$project_dir" >&2
    exit 1
fi

mkdir "$project_dir"

cat > "$project_dir/meson.build" <<EOF
project('$project_name',
        'cpp',
        version : '0.1.0',
        default_options : ['cpp_std=c++17', 'buildtype=debugoptimized'])

toolkit_dep = dependency('thorvg-toolkit')

executable(meson.project_name(),
           'main.cpp',
           dependencies : toolkit_dep)
EOF

cat > "$project_dir/main.cpp" <<EOF
#include <thorvg_toolkit.h>

struct MyApp : tvg::toolkit::App
{
    MyApp() : App("$project_name", {800, 600}) {}

    bool content(tvg::Canvas* canvas, const App::Size& size) override
    {
        auto shape = tvg::Shape::gen();
        shape->appendRect(0, 0, size.w, size.h);
        shape->fill(30, 120, 240);
        return canvas->add(shape) == tvg::Result::Success;
    }
};

int main()
{
    if (tvg::Initializer::init() != tvg::Result::Success) return 1;
    if (tvg::toolkit::run(new MyApp) != tvg::Result::Success) return 1;
    if (tvg::Initializer::term() != tvg::Result::Success) return 1;
}
EOF

printf 'Created project: %s\n\n' "$project_name"
printf 'Build and run (requires ThorVG Toolkit to be installed):\n'
printf '  cd %s\n  meson setup builddir\n  meson compile -C builddir\n  ./builddir/%s\n' "$project_name" "$project_name"
