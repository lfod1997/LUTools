from cmath import sqrt
from typing import Tuple

from PIL import Image
from PIL.Image import Resampling
import numpy as np

__all__ = [
    'map_channels',
    'should_flip',
    'color_to_position',
    'color_to_position_full',
    'block_begin_position',
    'generate_block',
    'generate_lutmap_full',
    'LUTMAP_FULL_SIZE',
    'LUTMAP_BLOCK_SIZE'
]

LUTMAP_FULL_SIZE = (4096, 4096)

LUTMAP_BLOCK_SIZE = (256, 256)

_lutmap_basic_tiles = []

_lutmap_tiles_initialized = False


def map_channels(axis: int):
    """Determines which channels are mapped to be the lutmap's axis, horizontal and vertical

    Parameters
    ----------
    axis : int
        Axis channel (unnormalized, can be negative)

    Returns
    -------
    tuple[int, int, int]
        [0]: axis channel, whose value enumerates, tile by tile, throughout the entire lutmap
        [1]: horizontal channel, whose value enumerates along the width of each "tile"
        [2]: vertical channel, whose value enumerates along the height of each "tile"
    """
    return (axis % 3, (axis + 1) % 3, (axis - 1) % 3)


def should_flip(stage: int):
    """Determines if a tile should be flipped, and how

    Parameters
    ----------
    stage : int
        A specific value on the axis channel

    Returns
    -------
    tuple[bool, bool]
        [0]: should flip horizontally
        [1]: should flip vertically
    """
    return (stage % 2 != 0, (stage // 16) % 2 != 0)


def block_begin_position(stage: int):
    """Returns the 2D position on the lutmap where the top-left pixel of a lut block should be placed (regardless of flipping)

    Parameters
    ----------
    stage : int
        A specific value on the axis channel

    Returns
    -------
    tuple[int, int]
        (x, y): the calculated position, can be used with `Image.getpixel()`
    """
    return ((stage % 16) * LUTMAP_BLOCK_SIZE[0], (stage // 16) * LUTMAP_BLOCK_SIZE[1])


def color_to_position(color: Tuple[int, int, int], res: int = 256, axis: int = 2, flip: bool = True, agrid: Tuple[int, int] = None):
    """Maps a color to a unique 2D position

    Parameters
    ----------
    color : MUST BE tuple[int, int, int]
        8-BIT (0~255) QUANTIZED RGB values, in the order of (R, G, B)
    res : int, optional
        RGB resolution for non-axis channels, by default 256 (0~255)
    axis : int, optional
        Axis channel, whose value enumerates, tile by tile, throughout the entire lutmap: 0 for R, 1 for G, 2 for B; by default 2
    flip : bool, optional
        Whether or not the tile "flipping" (used to lower the spatial frequency and thereby reduce JPEG noise) are taken into consideration, by default True
    agrid: tuple[int, int] | None, optional (not well-tested, use at your own risk)
        [0]: how many lut blocks each row, by default determined from `res`
        [1]: how many lut blocks each column, by default determined from `res`

    Returns
    -------
    tuple[int, int]
        (x, y): the position `color` is mapped to, can be used with `Image.getpixel()`
    """
    a, h, v = map_channels(axis)
    hflip, vflip = should_flip(color[a])

    if not agrid:
        sqrt_int = int(sqrt(res).real)
        agrid = (sqrt_int, sqrt_int)

    loss_fac_a = 256 // (agrid[0] * agrid[1])
    loss_fac_hv = 256 // res
    max_val_hv = res - 1

    resampled = (
        int(color[a] * (1.0 / loss_fac_a)),
        int(color[h] * (1.0 / loss_fac_hv)),
        int(color[v] * (1.0 / loss_fac_hv))
    )

    return (
        (resampled[0] % agrid[0]) * res +
        (max_val_hv - resampled[1] if flip and hflip else resampled[1]),
        (resampled[0] // agrid[0]) * res +
        (max_val_hv - resampled[2] if flip and vflip else resampled[2])
    )


def color_to_position_full(color: Tuple[int, int, int], axis: int = 2, flip: bool = True):
    """Maps a color to a unique 2D position on the lutmap

    Parameters
    ----------
    see `color_to_position`

    Returns
    -------
    see `color_to_position`
    """
    return color_to_position(color, 256, axis, flip)


def _initialize_tiles():
    """We use a "tile" to avoid creating similar little blocks, speeding up the code
    """
    global _lutmap_basic_tiles, _lutmap_tiles_initialized

    # "run-once" guard
    if _lutmap_tiles_initialized:
        return

    # Construct 3 basic tiles for use with 3 possible axis channels
    for axis in range(3):
        a, h, v = map_channels(axis)
        _lutmap_basic_tiles.append(
            np.zeros((*LUTMAP_BLOCK_SIZE, 3), dtype=np.uint8))
        for hvalue in range(LUTMAP_BLOCK_SIZE[0]):
            for vvalue in range(LUTMAP_BLOCK_SIZE[1]):
                color = [0, 0, 0]
                color[h] = hvalue
                color[v] = vvalue
                y, x = color_to_position_full(tuple(color), a, False)
                _lutmap_basic_tiles[a][x, y] = color

    _lutmap_tiles_initialized = True


def generate_lutmap_full(resize_fac: int = 1, axis: int = 2, flip: bool = True):
    """Generates a complete lutmap

    Parameters
    ----------
    resize_fac : int, optional
        Size multiplier, in case you want a bigger image, by default 1
    axis : int, optional
        Axis channel, whose value enumerates, tile by tile, throughout the entire lutmap: 0 for R, 1 for G, 2 for B; by default 2
    flip : bool, optional
        If or not, tile "flipping" (used to lower the spatial frequency and thereby reduce JPEG noise) are taken into consideration, by default True

    Returns
    -------
    Image.Image
        The generated image
    """
    global _lutmap_basic_tiles

    UNIT = np.ones(LUTMAP_BLOCK_SIZE, dtype=np.uint8)
    _initialize_tiles()

    # Image data array
    ima = np.zeros((*LUTMAP_FULL_SIZE, 3), dtype=np.uint8)

    # Make sure of the axis channel
    a = map_channels(axis)[0]

    # Obtain a deep copy of the tile being used
    tile: np.ndarray = _lutmap_basic_tiles[a].copy()

    # Actually just range(256), we use double loop to provide "flip" support
    for row in range(16):
        for step in range(16):
            # Value on the axis channel
            stage = row * 16 + step

            # Locate where we should put the tile
            ybegin, xbegin = block_begin_position(stage)
            yend, xend = ybegin + LUTMAP_BLOCK_SIZE[1], xbegin + LUTMAP_BLOCK_SIZE[0]

            # Copy tile to that position
            ima[xbegin:xend, ybegin:yend] = tile

            # We'll let the tile increment instead of "tile[...] = UNIT * stage"
            tile[:, :, a] += UNIT

            # Each step the tile flips horizontally
            if flip:
                tile = np.fliplr(tile)

        # Each row the tile flips vertically
        if flip:
            tile = np.flipud(tile)

    # Convert to image
    img = Image.fromarray(ima, mode='RGB')

    # Resize if required
    if resize_fac == 1:
        return img
    width = LUTMAP_FULL_SIZE[0] * resize_fac
    return img.resize((width, width), Resampling.NEAREST)


def generate_block(stage: int, resize_fac: int = 1, axis: int = 2):
    """Generates a lut block on the lutmap (i.e. a piece of the lutmap on which the value on the axis channel stays the same)

    Parameters
    ----------
    stage : int
        A specific value on the axis channel
    resize_fac : int, optional
        Size multiplier, in case you want a bigger image, by default 1
    axis : int, optional
        Which channel is the block's axis channel, by default 2

    Returns
    -------
    Image.Image
        The generated image
    
    Note
    ----
    This function is NOT a dependency of `generate_lutmap_full`
    """
    global _lutmap_basic_tiles

    _initialize_tiles()

    a = map_channels(axis)[0]
    ima: np.ndarray = _lutmap_basic_tiles[a].copy()
    ima[:, :, a] = np.ones(LUTMAP_BLOCK_SIZE, dtype=np.uint8) * stage

    img = Image.fromarray(ima, mode='RGB')

    # Resize if required
    if resize_fac == 1:
        return img
    width = LUTMAP_BLOCK_SIZE[0] * resize_fac
    return img.resize((width, width), Resampling.NEAREST)


if __name__ == '__main__':
    import sys

    try:
        resize_fac = int(sys.argv[1])
    except:
        resize_fac = 1

    generate_lutmap_full(resize_fac, axis=2).save(
        f'lutmap{resize_fac * LUTMAP_FULL_SIZE[0]}.png')
    generate_lutmap_full(resize_fac, axis=1).save(
        f'lutmap{resize_fac * LUTMAP_FULL_SIZE[0]}.g.png')
    generate_lutmap_full(resize_fac, axis=0).save(
        f'lutmap{resize_fac * LUTMAP_FULL_SIZE[0]}.r.png')

    # 9 * 256 ^ 2 sampling maps, deprecated :D
    # resize_fac = 16
    # for axis in range(3):
    #     for stage in (0, 128, 255):
    #         generate_block(stage, resize_fac, axis).save(
    #             f'lutmap_{"r" if axis == 0 else "g" if axis == 1 else "b"}{stage}.png')
