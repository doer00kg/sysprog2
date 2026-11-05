from pathlib import Path
from collections import Counter

# Файли записів лежать у docs/captures
folder = Path(__file__).resolve().parent.parent / 'docs' / 'captures'
files = ['i2c-uart-boot.vcd', 'pwm-two-angles.vcd']


def read_vcd(filename):
    # Читаємо час і зміни стану чотирьох каналів
    signals = {'!': [], '"': [], '#': [], '$': []}
    time = 0
    with open(filename, encoding='utf-8') as f:
        for line in f:
            if line.startswith('#'):
                time = int(line[1:])
            elif len(line) > 1 and line[0] in '01' and line[1] in signals:
                signals[line[1]].append((time, int(line[0])))
    return signals, time


def pulses(edges, state):
    # Тривалості ділянок з потрібним рівнем, у мікросекундах
    result = []
    for (t1, v1), (t2, v2) in zip(edges, edges[1:]):
        if v1 == state and v2 != state:
            result.append((t1 / 1_000_000, (t2 - t1) / 1000))
    return result


def middle(numbers):
    numbers = sorted(numbers)
    return numbers[len(numbers) // 2]


for name in files:
    path = folder / name
    if not path.exists():
        print('Немає файлу:', path)
        continue

    signals, last_time = read_vcd(path)
    print('\nФайл:', name)
    print(f'Тривалість: {last_time / 1_000_000_000:.3f} с')

    # D0 - SCL. Рахуємо тільки звичайні такти (8-12 мкс)
    scl = signals['!']
    rises = [t for t, value in scl if value == 1]
    cycles = [(b - a) / 1000 for a, b in zip(rises, rises[1:])]
    normal = [x for x in cycles if 8 <= x <= 12]
    if normal:
        period = middle(normal)
        print(f'I2C SCL: {1000 / period:.1f} кГц (такт {period:.3f} мкс)')

    # Паузи SCL понад 40 мс під час старту
    stops = [(start, width / 1000) for start, width in pulses(scl, 0) if width >= 40000]
    print('Довгі паузи SCL:', len(stops))
    for start, width in stops:
        print(f'  від {start:.3f} мс, тривалість {width:.3f} мс')

    # D2 - PWM. Ширина високого рівня та період між фронтами
    pwm = signals['#']
    widths = [width for _, width in pulses(pwm, 1)]
    starts = [t for t, value in pwm if value == 1]
    periods = [(b - a) / 1000 for a, b in zip(starts, starts[1:])]
    print('Кількість повних PWM-імпульсів:', len(widths))
    if periods:
        print(f'Період PWM: {middle(periods):.3f} мкс')
    counts = Counter(round(w) for w in widths[1:])
    print('Найчастіші ширини PWM (мкс, кількість):', counts.most_common(8))

    print('UART: перевірений вручну в PulseView (115200, 8N1)')

print('\nПримітка: причину довгих пауз SCL цей скрипт не встановлює.')
