namespace {
using Coord        = Eigen::Array<u8, 2, 1>;
using BeamGrid     = std::mdspan<u8, std::dextents<u32, 2>>;
using MirrorGrid   = std::mdspan<const char, std::dextents<u32, 2>, std::layout_stride>;
constexpr u8 RIGHT = (1 << 0), DOWN = (1 << 2), LEFT = (1 << 4), UP = (1 << 6);
constexpr u8 RIGHT_OR_LEFT  = RIGHT | LEFT;
constexpr u8 UP_OR_DOWN     = DOWN | UP;
constexpr u8 DIRECTION_MASK = RIGHT | DOWN | LEFT | UP;

constexpr void AssertIsDirection(u8 direction) {
    Assume(std::has_single_bit(direction) && (direction & DIRECTION_MASK));
}
constexpr u8 Right(u8 direction) {
    AssertIsDirection(direction);
    return std::rotl(direction, 2);
}
constexpr u8 Left(u8 direction) {
    AssertIsDirection(direction);
    return std::rotr(direction, 2);
}
const std::array<Coord, 4> DIRECTION_OFFSETS{{
    Coord{0, 1},
    Coord{1, 0},
    Coord{0, -1},
    Coord{-1, 0},
}};
const std::array<Coord, 128> DIRECTION_OFFSETS2 = [] {
    std::array<Coord, 128> tmp{};
    tmp[RIGHT] = DIRECTION_OFFSETS[0];
    tmp[DOWN]  = DIRECTION_OFFSETS[1];
    tmp[LEFT]  = DIRECTION_OFFSETS[2];
    tmp[UP]    = DIRECTION_OFFSETS[3];
    return tmp;
}();
Coord DirectionOffset(u8 direction) {
    AssertIsDirection(direction);
    return DIRECTION_OFFSETS2[direction];
}

auto MakeGrid(std::string_view input) {
    const u8 width  = input.find('\n');
    const u8 stride = width + 1;
    const u8 height = input.size() / stride;
    std::mdspan<const char, std::dextents<u32, 2>> mirrorsWithNewlines{input.data(), height, stride};
    const MirrorGrid mirrors =
        std::submdspan(mirrorsWithNewlines, std::full_extent, std::pair{std::integral_constant<u32, 0>{}, width});
    std::vector<u8> beamStorage(mirrors.size());
    BeamGrid beamTiles{beamStorage.data(), mirrors.extents()};
    return std::make_tuple(mirrors, std::move(beamStorage), beamTiles);
}

bool TestAndSetBeamDirection(u8& tile, u8 direction) {
    u8 beamDirections     = tile;
    bool alreadyTraversed = beamDirections & direction;
    tile                  = beamDirections | direction;
    return alreadyTraversed;
}

void TraverseBeam(const MirrorGrid& mirrors, const BeamGrid& beamTiles, Coord coord, u8 direction) {
    if (coord[0] >= beamTiles.extent(0) || coord[1] >= beamTiles.extent(1)) {
        return;
    }
    const char mirror = mirrors[ToSpan(coord)];
    auto& beams       = beamTiles[ToSpan(coord)];
    switch (mirror) {
        case '.': {
            if (TestAndSetBeamDirection(beams, direction)) return;
            [[clang::musttail]] return TraverseBeam(mirrors, beamTiles, coord + DirectionOffset(direction), direction);
        }
        case '/': {
            const u8 exitDirection = (direction & RIGHT_OR_LEFT) ? Left(direction) : Right(direction);
            if (TestAndSetBeamDirection(beams, exitDirection)) return;
            [[clang::musttail]] return TraverseBeam(mirrors, beamTiles, coord + DirectionOffset(exitDirection),
                                                    exitDirection);
        }
        case '\\': {
            const u8 exitDirection = (direction & UP_OR_DOWN) ? Left(direction) : Right(direction);
            if (TestAndSetBeamDirection(beams, exitDirection)) return;
            [[clang::musttail]] return TraverseBeam(mirrors, beamTiles, coord + DirectionOffset(exitDirection),
                                                    exitDirection);
        }
        case '|': {
            if (direction & RIGHT_OR_LEFT) {
                if (!TestAndSetBeamDirection(beams, DOWN)) {
                    TraverseBeam(mirrors, beamTiles, coord + DirectionOffset(DOWN), DOWN);
                }
                if (!TestAndSetBeamDirection(beams, UP)) {
                    [[clang::musttail]] return TraverseBeam(mirrors, beamTiles, coord + DirectionOffset(UP), UP);
                }
                return;
            } else {
                if (TestAndSetBeamDirection(beams, direction)) return;
                [[clang::musttail]] return TraverseBeam(mirrors, beamTiles, coord + DirectionOffset(direction),
                                                        direction);
            }
        }
        case '-': {
            if (direction & UP_OR_DOWN) {
                if (!TestAndSetBeamDirection(beams, RIGHT)) {
                    TraverseBeam(mirrors, beamTiles, coord + DirectionOffset(RIGHT), RIGHT);
                }
                if (!TestAndSetBeamDirection(beams, LEFT)) {
                    [[clang::musttail]] return TraverseBeam(mirrors, beamTiles, coord + DirectionOffset(LEFT), LEFT);
                }
                return;
            } else {
                if (TestAndSetBeamDirection(beams, direction)) return;
                [[clang::musttail]] return TraverseBeam(mirrors, beamTiles, coord + DirectionOffset(direction),
                                                        direction);
            }
        }
        default: throw std::logic_error{"invalid mirror space"};
    }
}
u64 CountEnergized(const std::span<const u8> beamTiles) {
    return ranges::count_if(beamTiles, [](u8 beamTile) { return beamTile != 0; });
}

u64 Part1(const MirrorGrid& mirrors, const BeamGrid& beamTiles, const std::span<u8> beamTiles1d) {
    TraverseBeam(mirrors, beamTiles, Coord{0, 0}, RIGHT);
    return CountEnergized(beamTiles1d);
}

u64 Part2(const MirrorGrid& mirrors, const BeamGrid& beamTiles, const std::span<u8> beamTiles1d) {
    u64 maxTiles{};
    for (usize y : views::iota(0_sz, mirrors.extent(0))) {
        ranges::fill(beamTiles1d, u8{});
        TraverseBeam(mirrors, beamTiles, Coord{y, 0}, RIGHT);
        maxTiles = std::max(maxTiles, CountEnergized(beamTiles1d));
        ranges::fill(beamTiles1d, u8{});
        TraverseBeam(mirrors, beamTiles, Coord{y, mirrors.extent(1) - 1}, LEFT);
        maxTiles = std::max(maxTiles, CountEnergized(beamTiles1d));
    }
    for (usize x : views::iota(0_sz, mirrors.extent(1))) {
        ranges::fill(beamTiles1d, u8{});
        TraverseBeam(mirrors, beamTiles, Coord{0, x}, DOWN);
        maxTiles = std::max(maxTiles, CountEnergized(beamTiles1d));
        ranges::fill(beamTiles1d, u8{});
        TraverseBeam(mirrors, beamTiles, Coord{mirrors.extent(0) - 1, x}, UP);
        maxTiles = std::max(maxTiles, CountEnergized(beamTiles1d));
    }
    return maxTiles;
}

} // namespace

void AocMain(std::string_view input) {
    for (int rep = 0; rep < 100; ++rep) {
        auto [mirrors, beamStorage, beamTiles] = StopWatch<std::micro>::Run("Setting up mirrors", MakeGrid, input);
        logger.solution("Part 1: {}",
                        StopWatch<std::micro>::Run("Part 1", Part1, mirrors, beamTiles, std::span{beamStorage}));
        logger.solution("Part 2: {}",
                        StopWatch<std::milli>::Run("Part 2", Part2, mirrors, beamTiles, std::span{beamStorage}));
    }
}