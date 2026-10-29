"""Run SDL's actual event/render loop without a display or changing user saves."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

binary = str(Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory(prefix='last-watch-gui-') as folder:
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
    result = subprocess.run([binary, '--smoke-test', '--seed', '42', '--screenshot', 'board.bmp'],
                            cwd=folder, env=env, capture_output=True, text=True, timeout=60)
    assert result.returncode == 0, result.stdout + result.stderr
    image = Path(folder, 'board.bmp')
    assert image.exists() and image.stat().st_size > 1_000_000, 'Board screenshot missing'
    for stage in ['card', 'combat', 'enemy-combat', 'last-stand', 'icons', 'trade', 'market', 'bridge-warning', 'bridge-collapse', 'title', 'active-defender', 'manual-combat', 'manual-bridge']:
        assert Path(folder, f'board.bmp.{stage}.bmp').stat().st_size > 1_000_000, f'{stage} screenshot missing'
    assert Path(folder, 'gui named patrol.sav').exists(), 'GUI did not save'
    print(result.stdout)
