#!python3
import ctsBackend
import argparse
import threading
from pathlib import Path
from tabulate import tabulate

logger_mutex = threading.Lock()

def anari_logger(message):
    with logger_mutex:
        with open("ANARI.log", 'a') as file:
            file.write(message)
    #print(message)

def check_core_extensions(anari_library, anari_device = None):
    return ctsBackend.check_core_extensions(anari_library, anari_device, anari_logger)

def render_scenes(anari_library, anari_device = None, anari_renderer = "default", test_scenes = "all", output = "."):
    collected_scenes = []
    # TODO: collect scenes
    image_data = ctsBackend.render_scenes(anari_library, anari_device, anari_renderer, collected_scenes, anari_logger)
    # TODO: write to file


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='ANARI CTS toolkit')
    subparsers = parser.add_subparsers(dest="command", title='Commands', metavar=None)

    parentParser = argparse.ArgumentParser(add_help=False)
    parentParser.add_argument('library', help='ANARI library to load')
    parentParser.add_argument('--device', default=None, help='ANARI device on which to perform the test')

    renderScenesParser = subparsers.add_parser('render_scenes', description='Renders an image to disk for each test scene', parents=[parentParser])
    renderScenesParser.add_argument('--renderer', default="default")
    renderScenesParser.add_argument('--test_scenes', default="all")
    renderScenesParser.add_argument('--output', default=".")

    checkExtensionsParser = subparsers.add_parser('check_core_extensions', parents=[parentParser])

    command_text = ""
    for subparser in subparsers.choices :
        command_text += subparser + "\n  "
    subparsers.metavar = command_text

    args = parser.parse_args()

    if args.command == "render_scenes":
        render_scenes(args.library, args.device, args.renderer, args.test_scenes, args.output)
    elif args.command == "check_core_extensions":
        result = check_core_extensions(args.library, args.device)
        print(tabulate(result))