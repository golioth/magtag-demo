'''my_west_extension.py

Basic example of a west extension.'''

from textwrap import dedent            # just for nicer code indentation

from west.commands import WestCommand  # your extension must subclass this
from west import log                   # use this for user output

class KASM(WestCommand):

    def __init__(self):
        super().__init__(
            'kasm',  # gets stored as self.name
            'Help commands using a Kasm container for Golioth developer training',  # self.help
            # self.description:
            dedent('''
            Golioth developer training includes the option of using a docker
            container with the Zepyr tools that is accessible in the browser
            using the Kasm platform. However, west flash cannot be used because
            the USB port isn\'t available in the container.

            Using west kasm download merges the compiled binaries and places them
            into the Kasm download folder. By default, the output is placed in 
            ~/Desktop/Downloads as this is a predefined location in Kasm. The
            output directory can be specified using the -o flag.'''))

    def do_add_parser(self, parser_adder):
        # This is a bit of boilerplate, which allows you full control over the
        # type of argparse handling you want. The "parser_adder" argument is
        # the return value of an argparse.ArgumentParser.add_subparsers() call.
        parser = parser_adder.add_parser(self.name,
                                         help=self.help,
                                         description=self.description)

        # Add some example options using the standard argparse module API.
        parser.add_argument('command', choices=['download'],
                help='merge binaries and move to kasm download folder')

        parser.add_argument('-d', '--build-dir', default='build', help='build directory to use')
        parser.add_argument('-o', '--output-dir', default='~/Desktop/Downloads', help='output directory to use')

        return parser           # gets stored as self.parser

    def do_run(self, args, unknown_args):
        # This gets called when the user runs the command, e.g.:
        #
        #   $ west my-command-name -o FOO BAR
        #   --optional is FOO
        #   required is BAR
        if args.command == "download":
            import os, time

            #os.path.join only works if build_dir doesn't have a leading slash
            build_dir = os.path.join(os.getcwd(), os.path.expanduser(args.build_dir))
            output_dir = os.path.join(os.getcwd(), os.path.expanduser(args.output_dir))

            if not os.path.exists(build_dir):
                log.die('cannot find build directory: ', build_dir)
            if not os.path.exists(output_dir):
                log.die('cannot find output directory: ', output_dir)

            merged_filename = time.strftime('merged_%y%m%d_%H%M%S.bin', time.localtime())
            merged_path=os.path.join(output_dir, merged_filename)
            zephyrbin_path=os.path.join(build_dir, 'zephyr/zephyr.bin')
            bootloader_path=os.path.join(build_dir, 'esp-idf/build/bootloader/bootloader.bin')
            partitions_path=os.path.join(build_dir, 'esp-idf/build/partitions_singleapp.bin')


            if not os.path.exists(zephyrbin_path):
                log.die('cannot find zephyr.bin: ', zephyrbin_path)
            if not os.path.exists(bootloader_path):
                log.die('cannot find bootloader.bin: ', bootloader_path)
            if not os.path.exists(partitions_path):
                log.die('cannot find partitions_singleapp.bin: ', partitions_path)

            esptool_args = ['--chip', 'esp32s2', 'merge_bin', '-o', merged_path, '--flash_mode', 'dio', '--flash_freq', 'keep', '--flash_size', 'keep', '0x1000', bootloader_path, '0x8000', partitions_path, '0x10000', zephyrbin_path]

            try:
                import esptool
            except:
                log.die("esptool is not installed, please run: pip install esptool")
            esptool.main(esptool_args)
