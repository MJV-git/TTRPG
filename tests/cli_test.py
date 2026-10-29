"""Exercise the real terminal entrypoint, with isolated saves and bounded execution."""
import pathlib
import subprocess
import sys
import tempfile

binary = str(pathlib.Path(sys.argv[1]).resolve())

def run(args, commands, cwd, expected=0):
    result = subprocess.run([binary, '--no-color', *args], input=commands, text=True,
                            capture_output=True, cwd=cwd, timeout=15)
    assert result.returncode == expected, (result.returncode, result.stdout, result.stderr)
    return result.stdout + result.stderr

with tempfile.TemporaryDirectory(prefix='last-watch-test-') as directory:
    root = pathlib.Path(directory)
    output = run(['--quick', '--seed', '42'],
                 'move 999\nattack 999\nequip 999\nmove 2 trailing\nhelp\ninventory\nsearch\nsave patrol with spaces.sav\nquit\n', directory)
    assert 'No action spent' in output
    assert 'Unknown order or extra arguments' in output
    assert 'No action spent' in output
    assert 'ROUND 1 COMPLETE' in output
    assert (root / 'patrol with spaces.sav').is_file()
    assert not (root / 'patrol with spaces.sav.tmp').exists()
    # Quit saves to the most recently selected path, not an unrelated default.
    saved = (root / 'patrol with spaces.sav').read_text()
    output = run(['--load', 'patrol with spaces.sav'], 'status\nquit\n', directory)
    assert 'Campaign restored' in output and 'Round 2' in output
    assert (root / 'patrol with spaces.sav').read_text() == saved
    # Loading a corrupt file reports an error and preserves the active game.
    (root / 'bad.sav').write_text('FORTY_K_RPG 1\n0 broken')
    output = run(['--load', 'patrol with spaces.sav'], 'load bad.sav\nstatus\nquit\n', directory)
    assert 'Invalid save seed' in output and 'YOUR ORDER' in output
    assert (root / 'patrol with spaces.sav').read_text() == saved
    output = run(['--load', 'missing.sav'], '', directory, expected=1)
    assert 'Cannot open save' in output
    output = run(['--seed', '-1'], '', directory, expected=1)
    assert 'unsigned 32-bit' in output
    # EOF is a graceful save, including in a piped/noninteractive invocation.
    output = run(['--quick', '--seed', '0'], '', directory)
    assert 'Campaign saved' in output and (root / 'campaign.sav').is_file()
    output = run([], '1\n4\n2\n5\nCustom Defender\n2\n9 9 9 9 9\n4 4 4 4 4\n3\nSeer\nwait\nquit\n', directory)
    assert 'Invalid stats or budget exceeded' in output
    assert 'P2 Seer' in output
    assert 'ROUND 1 COMPLETE' not in output  # Only player one's action was spent.
    output = run([], '1\n1\n2\n1\nTrader\n2\nFriend\ntrade 2 medkit\ntrade 2 weapon -2147483648\nmarket\nmarket -2147483648\ncollapse\ncollapse confirm\nquit\n', directory)
    assert 'Trader gives Medkit to Friend' in output or 'Trader gives medkit to Friend' in output
    assert 'PERMANENTLY KILLS' in output and 'Offers refresh each defender turn' in output
    assert 'ROUND 1 COMPLETE' not in output
    # Existing temporary files are not overwritten; save failure keeps the game open.
    (root / 'blocked.sav.tmp').write_text('preserve me')
    output = run(['--quick'], 'save blocked.sav\nstatus\nquit\n', directory)
    assert 'Temporary save already exists' in output
    assert (root / 'blocked.sav.tmp').read_text() == 'preserve me'
    # A no-action waiting strategy reaches a real ending and exits without hanging.
    output = run(['--quick', '--seed', '123'], 'wait\n' * 100, directory)
    assert 'Debrief:' in output and ('THE CITY FALLS' in output or 'THE LAST BELL' in output)

print('PASS terminal setup, commands, saves, failed loads, EOF, and campaign defeat')
