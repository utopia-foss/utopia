#!/usr/bin/env python
"""This is the command line interface for Utopia"""

import argparse

import utopya

# -----------------------------------------------------------------------------
if __name__ == '__main__':
    # Define the CLI
    parser = argparse.ArgumentParser(description='Perform a Utopia run.',
                                     epilog="And that's how you use utopia.")
    parser.add_argument('model_name',
                        help="The name of the model to run")
    parser.add_argument('run_cfg',
                        help="The path to the run configuration")
    parser.add_argument('-s', '--single',
                        default=None, action='store_true',
                        help="If given, forces a single simulation.")
    parser.add_argument('-p', '--sweep',
                        default=None, action='store_true',
                        help="If given, forces a parameter sweep. Is ignored, "
                             "if the --single argument was also given.")
    parser.add_argument('-v', '--verbose',
                        default=0, action='count',
                        help="(UNSUPPORTED) Sets the verbosity level of the "
                             "frontend.")

    # CLI defined. Parse the arguments now.
    args = parser.parse_args()

    # Using the args, instantiate a Multiverse object
    mv = utopya.Multiverse(model_name=args.model_name,
                           run_cfg=args.run_cfg)
