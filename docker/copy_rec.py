#!/env/bin/python3

import os
import glob
import shutil
import argparse

def copy_files(args):
    pathname = os.path.join(args.input_dir, args.pattern)
    file_paths = glob.glob(pathname, recursive=args.recursive)
    if not file_paths:
        raise RuntimeError("Nothing to copy!")

    for input_path in file_paths:
        dest_path = os.path.join(args.output_dir,
                                 os.path.relpath(input_path,
                                                 start=args.input_dir))
        os.makedirs(os.path.dirname(dest_path), exist_ok=True)
        new_path = shutil.copy(input_path, dest_path, follow_symlinks=True)

        if args.verbose:
            print("{} -> {}".format(input_path, new_path))

def parse_args():
    parser = argparse.ArgumentParser(
        description="Recursively copy files based on a pattern")
    parser.add_argument("pattern",
                        help="The file pattern to copy",
                        type=str)
    parser.add_argument("input_dir",
                        help="Input directory to search in",
                        type=os.path.realpath)
    parser.add_argument("output_dir",
                        help="Output directory to place files into",
                        type=os.path.realpath)
    parser.add_argument("--recursive", "-r",
                        help="Perform a recursive search",
                        action="store_true")
    parser.add_argument("--verbose", "-v",
                        help="Print which files are copied",
                        action="store_true")
    return parser.parse_args()

if __name__ == "__main__":
    args = parse_args()
    copy_files(args)
