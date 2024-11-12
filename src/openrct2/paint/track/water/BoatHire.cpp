/*****************************************************************************
/*****************************************************************************
 * Copyright (c) 2014-2024 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "../../../interface/Viewport.h"
#include "../../../ride/Ride.h"
#include "../../../ride/Track.h"
#include "../../../ride/TrackPaint.h"
#include "../../Paint.h"
#include "../../tile_element/Segment.h"
#include "../../track/Segment.h"
#include "../../track/Support.h"

using namespace OpenRCT2;

enum
{
    SPR_BOAT_HIRE_FLAT_BACK_SW_NE = 28523,
    SPR_BOAT_HIRE_FLAT_FRONT_SW_NE = 28524,
    SPR_BOAT_HIRE_FLAT_BACK_NW_SE = 28525,
    SPR_BOAT_HIRE_FLAT_FRONT_NW_SE = 28526,
    SPR_BOAT_HIRE_FLAT_QUARTER_TURN_1_TILE_BACK_SW_NW = 28527,
    SPR_BOAT_HIRE_FLAT_QUARTER_TURN_1_TILE_FRONT_SW_NW = 28528,
    SPR_BOAT_HIRE_FLAT_QUARTER_TURN_1_TILE_BACK_NW_NE = 28529,
    SPR_BOAT_HIRE_FLAT_QUARTER_TURN_1_TILE_FRONT_NW_NE = 28530,
    SPR_BOAT_HIRE_FLAT_QUARTER_TURN_1_TILE_BACK_NE_SE = 28531,
    SPR_BOAT_HIRE_FLAT_QUARTER_TURN_1_TILE_FRONT_NE_SE = 28532,
    SPR_BOAT_HIRE_FLAT_QUARTER_TURN_1_TILE_FRONT_SE_SW = 28533,
    SPR_BOAT_HIRE_FLAT_QUARTER_TURN_1_TILE_BACK_SE_SW = 28534,
};

/** rct2: 0x008B0E40 */
static void PaintBoatHireTrackFlat(
    PaintSession& session, const Ride& ride, uint8_t trackSequence, uint8_t direction, int32_t height,
    const TrackElement& trackElement, SupportType supportType)
{
    ImageId imageId;

    const auto offset = CoordsXYZ{ 0, 0, height };
    if (direction & 1)
    {
        imageId = session.TrackColours.WithIndex(SPR_BOAT_HIRE_FLAT_BACK_NW_SE);
        PaintAddImageAsParent(session, imageId, offset, { { 4, 0, height }, { 1, 32, 3 } });

        imageId = session.TrackColours.WithIndex(SPR_BOAT_HIRE_FLAT_FRONT_NW_SE);
        PaintAddImageAsParent(session, imageId, offset, { { 28, 0, height }, { 1, 32, 3 } });
    }
    else
    {
        imageId = session.TrackColours.WithIndex(SPR_BOAT_HIRE_FLAT_BACK_SW_NE);
        PaintAddImageAsParent(session, imageId, offset, { { 0, 4, height }, { 32, 1, 3 } });

        imageId = session.TrackColours.WithIndex(SPR_BOAT_HIRE_FLAT_FRONT_SW_NE);
        PaintAddImageAsParent(session, imageId, offset, { { 0, 28, height }, { 32, 1, 3 } });
    }

    PaintUtilSetSegmentSupportHeight(
        session,
        PaintUtilRotateSegments(
            EnumsToFlags(PaintSegment::bottomLeftSide, PaintSegment::centre, PaintSegment::topRightSide), direction),
        0xFFFF, 0);
    PaintUtilSetGeneralSupportHeight(session, height + 16);
}

/** rct2: 0x008B0E50 */
static void PaintBoatHireStation(
    PaintSession& session, const Ride& ride, uint8_t trackSequence, uint8_t direction, int32_t height,
    const TrackElement& trackElement, SupportType supportType)
{
    const auto* stationObj = ride.GetStationObject();

    TrackPaintUtilDrawStationTunnel(session, direction, height);
    TrackPaintUtilDrawPier(
        session, ride, stationObj, session.MapPosition, direction, height, trackElement, session.CurrentRotation);

    PaintUtilSetSegmentSupportHeight(session, kSegmentsAll, 0xFFFF, 0);
    PaintUtilSetGeneralSupportHeight(session, height + kDefaultGeneralSupportHeight);
}

/** rct2: 0x008B0E80 */
static void PaintBoatHireTrackLeftQuarterTurn1Tile(
    PaintSession& session, const Ride& ride, uint8_t trackSequence, uint8_t direction, int32_t height,
    const TrackElement& trackElement, SupportType supportType)
{
    ImageId imageId;
    const auto offset = CoordsXYZ{ 0, 0, height };
    switch (direction)
    {
        case 0:
            imageId = session.TrackColours.WithIndex(SPR_BOAT_HIRE_FLAT_QUARTER_TURN_1_TILE_BACK_SW_NW);
            PaintAddImageAsParent(session, imageId, offset, { { 0, 0, height }, { 32, 32, 0 } });

            imageId = session.TrackColours.WithIndex(SPR_BOAT_HIRE_FLAT_QUARTER_TURN_1_TILE_FRONT_SW_NW);
            PaintAddImageAsParent(session, imageId, offset, { { 28, 28, height + 2 }, { 3, 3, 3 } });
            break;
        case 1:
            imageId = session.TrackColours.WithIndex(SPR_BOAT_HIRE_FLAT_QUARTER_TURN_1_TILE_BACK_NW_NE);
            PaintAddImageAsParent(session, imageId, offset, { { 0, 0, height }, { 32, 32, 0 } });

            imageId = session.TrackColours.WithIndex(SPR_BOAT_HIRE_FLAT_QUARTER_TURN_1_TILE_FRONT_NW_NE);
            PaintAddImageAsParent(session, imageId, offset, { { 28, 28, height + 2 }, { 3, 3, 3 } });
            break;
        case 2:
            imageId = session.TrackColours.WithIndex(SPR_BOAT_HIRE_FLAT_QUARTER_TURN_1_TILE_BACK_NE_SE);
            PaintAddImageAsParent(session, imageId, offset, { { 0, 0, height }, { 32, 32, 0 } });

            imageId = session.TrackColours.WithIndex(SPR_BOAT_HIRE_FLAT_QUARTER_TURN_1_TILE_FRONT_NE_SE);
            PaintAddImageAsParent(session, imageId, offset, { { 28, 28, height + 2 }, { 3, 3, 3 } });
            break;
        case 3:
            imageId = session.TrackColours.WithIndex(SPR_BOAT_HIRE_FLAT_QUARTER_TURN_1_TILE_FRONT_SE_SW);
            PaintAddImageAsParent(session, imageId, offset, { { 28, 28, height + 2 }, { 3, 3, 3 } });

            imageId = session.TrackColours.WithIndex(SPR_BOAT_HIRE_FLAT_QUARTER_TURN_1_TILE_BACK_SE_SW);
            PaintAddImageAsParent(session, imageId, offset, { { 0, 0, height }, { 32, 32, 0 } });
            break;
    }

    PaintUtilSetSegmentSupportHeight(
        session,
        PaintUtilRotateSegments(
            EnumsToFlags(PaintSegment::bottomLeftSide, PaintSegment::centre, PaintSegment::topLeftSide), direction),
        0xFFFF, 0);
    PaintUtilSetGeneralSupportHeight(session, height + 16);
}

/** rct2: 0x008B0E90 */
static void PaintBoatHireTrackRightQuarterTurn1Tile(
    PaintSession& session, const Ride& ride, uint8_t trackSequence, uint8_t direction, int32_t height,
    const TrackElement& trackElement, SupportType supportType)
{
    PaintBoatHireTrackLeftQuarterTurn1Tile(
        session, ride, trackSequence, (direction + 3) % 4, height, trackElement, supportType);
}

/**
 * rct2: 0x008B0D60
 */
TRACK_PAINT_FUNCTION GetTrackPaintFunctionBoatHire(OpenRCT2::TrackElemType trackType)
{
    switch (trackType)
    {
        case TrackElemType::Flat:
            return PaintBoatHireTrackFlat;

        case TrackElemType::EndStation:
        case TrackElemType::BeginStation:
        case TrackElemType::MiddleStation:
            return PaintBoatHireStation;

        case TrackElemType::LeftQuarterTurn1Tile:
            return PaintBoatHireTrackLeftQuarterTurn1Tile;
        case TrackElemType::RightQuarterTurn1Tile:
            return PaintBoatHireTrackRightQuarterTurn1Tile;
        default:
            return nullptr;
    }
}
