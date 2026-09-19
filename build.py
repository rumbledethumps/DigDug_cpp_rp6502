#!/usr/bin/env python3
"""Build (and optionally run) the RP6502 Dig Dug ROM.

Finds mos-rp6502-clang on the PATH, derives the llvm-mos SDK root from it,
configures CMake with the llvm-mos toolchain file, builds digdug.rp6502 and
copies it to rp6502/dist/.

    python build.py            build
    python build.py --run      build, then launch the emulator with the ROM
    python build.py --clean    remove the build directory first
"""
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
BUILD = os.path.join(HERE, 'build', 'llvm-mos')
DIST = os.path.join(HERE, 'dist')


def find_compiler():
    for name in ('mos-rp6502-clang', 'mos-rp6502-clang.bat', 'mos-rp6502-clang.exe'):
        p = shutil.which(name)
        if p:
            return p
    sys.exit('mos-rp6502-clang not found on PATH (install llvm-mos-sdk and add its bin/ to PATH)')


def find_ninja():
    p = shutil.which('ninja')
    if p:
        return p
    vs = os.path.join(os.environ.get('ProgramFiles', r'C:\Program Files'),
                      'Microsoft Visual Studio', '2022')
    if os.path.isdir(vs):
        for ed in os.listdir(vs):
            cand = os.path.join(vs, ed, 'Common7', 'IDE', 'CommonExtensions', 'Microsoft', 'CMake', 'Ninja', 'ninja.exe')
            if os.path.exists(cand):
                return cand
    return None


def main():
    argv = sys.argv[1:]
    if '--clean' in argv and os.path.isdir(BUILD):
        shutil.rmtree(BUILD)
    cc = find_compiler()
    sdk = os.path.dirname(os.path.dirname(cc))
    toolchain = os.path.join(sdk, 'lib', 'cmake', 'llvm-mos-sdk', 'llvm-mos-toolchain.cmake')
    if not os.path.exists(toolchain):
        sys.exit('toolchain file not found: ' + toolchain)
    ninja = find_ninja()
    gen = ['-G', 'Ninja'] if ninja else []
    env = dict(os.environ)
    if ninja:
        env['PATH'] = os.path.dirname(ninja) + os.pathsep + env['PATH']
    env['PATH'] = os.path.dirname(cc) + os.pathsep + env['PATH']
    cfg = ['cmake', '-S', HERE, '-B', BUILD, *gen,
           '-DLLVM_MOS_PLATFORM=rp6502',
           '-DCMAKE_TOOLCHAIN_FILE=' + toolchain.replace('\\', '/'),
           '-DCMAKE_BUILD_TYPE=Release']
    print(' '.join(cfg))
    subprocess.check_call(cfg, env=env)
    subprocess.check_call(['cmake', '--build', BUILD, '--target', 'digdug_rp6502'], env=env)
    rom = os.path.join(BUILD, 'digdug.rp6502')
    os.makedirs(DIST, exist_ok=True)
    out = os.path.join(DIST, 'digdug.rp6502')
    shutil.copyfile(rom, out)
    print('ROM:', out)
    if '--run' in argv:
        emu = os.path.join(HERE, 'emu', 'rp6502-emu.exe' if os.name == 'nt' else 'rp6502-emu')
        if not os.path.exists(emu):
            emu = shutil.which('rp6502-emu')
        if not emu:
            sys.exit('emulator not found (rp6502/emu/rp6502-emu.exe)')
        subprocess.call([emu, out])


if __name__ == '__main__':
    main()
