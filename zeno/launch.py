import runpy
import tempfile
import threading
import atexit
import shutil
import subprocess
import json
import sys
import os
from . import run
from multiprocessing import Process


g_proc = None
g_iopath = None


def killProcess():
    global g_proc
    if g_proc is None:
        print('worker process is not running')
        return
    g_proc.terminate()
    g_proc = None
    print('worker process killed')


@atexit.register
def cleanIOPath():
    global g_iopath
    if g_iopath is not None:
        iopath = g_iopath
        g_iopath = None
        shutil.rmtree(iopath, ignore_errors=True)


def launchProgram(prog, nframes):
    global g_iopath
    cleanIOPath()
    g_iopath = tempfile.mkdtemp(prefix='zenvis-')
    print('IOPath:', g_iopath)
    #_launch_mproc(run.runScene, prog['graph'], nframes, g_iopath)
    filepath = os.path.join(g_iopath, 'prog.zsg')
    with open(filepath, 'w') as f:
        json.dump(prog, f)
    subprocess.check_call([sys.executable, '-m', 'zeno', filepath, str(nframes), g_iopath])


def getDescriptors():
    descs = run.dumpDescriptors()
    descs = descs.splitlines()
    descs = [parse_descriptor_line(line) for line in descs if line.startswith('DESC:')]
    descs = {name: desc for name, desc in descs}
    print('Loaded', len(descs), 'descriptors')
    return descs


def parse_descriptor_line(line):
    _, z_name, rest = line.strip().split(':', maxsplit=2)
    assert rest.startswith('(') and rest.endswith(')'), (n_name, rest)
    inputs, outputs, params, categories = rest[1:-1].split(')(')

    z_inputs = [name for name in inputs.split(',') if name]
    z_outputs = [name for name in outputs.split(',') if name]
    z_categories = [name for name in categories.split(',') if name]

    z_params = []
    for param in params.split(','):
        if not param:
            continue
        type, name, defl = param.split(':')
        z_params.append((type, name, defl))

    z_desc = {
        'inputs': z_inputs,
        'outputs': z_outputs,
        'params': z_params,
        'categories': z_categories,
    }

    return z_name, z_desc


__all__ = [
    'getDescriptors',
    'launchProgram',
    'killProcess',
]
