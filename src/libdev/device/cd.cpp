/*
 * C D . C P P
 * (c) Charybdis Limited, 1998. All Rights Reserved
 */

//  Definitions of non-inline non-template methods and global functions
//
//  NOTE: The original Windows build used MCI CD-audio and the Linux port used
//  the `alure` streaming helper to play ogg music tracks.  `alure` is not
//  available on macOS, and music is delivered as downloadable content rather
//  than from a CD, so this file keeps the DevCD interface (backed by plain
//  OpenAL for volume/source state) but no longer streams tracks itself.

#include "afx/sdlapp.hpp"

#include "device/cd.hpp"
#include "device/cdlist.hpp"
#include "mathex/random.hpp"
#include "system/pathname.hpp"

class DevCDImpl
{
	enum CDVOLUME
	{
		MAX_CDVOLUME = 65535,
		MIN_CDVOLUME = 0
	};

    ALuint              source_;

	unsigned int savedVolume_;

	DevCDPlayList* pPlayList_;

	bool haveMixer_;

	DevCDTrackIndex randomStartTrack_;
	DevCDTrackIndex randomEndTrack_;
	MexBasicRandom	randomGenerator_;

	friend class DevCD;
};

// static
DevCD& DevCD::instance()
{
    static DevCD instance;
    return instance;
}

DevCD::DevCD():
status_(NORMAL),
needsUpdate_(false),
pImpl_(_NEW(DevCDImpl()))
{
    CB_DEPIMPL(ALuint, source_);
	CB_DEPIMPL(unsigned int, savedVolume_);
	CB_DEPIMPL(DevCDPlayList*, pPlayList_);
	CB_DEPIMPL(bool, haveMixer_);
	CB_DEPIMPL(MexBasicRandom, randomGenerator_);

	haveMixer_ = false;
	savedVolume_ = 20;

	bool noErrors = true;

    alGenSources(1, &source_);
    if(alGetError() != AL_NO_ERROR)
    {
        std::cerr << "Failed to create OpenAL source for music mixer!" << std::endl;
        noErrors = false;
    }
    haveMixer_ = noErrors;

	pPlayList_ = _NEW(DevCDPlayList(numberOfTracks()));

	randomGenerator_.seedFromTime();
}

DevCD::~DevCD()
{
    CB_DEPIMPL(ALuint, source_);
	CB_DEPIMPL(unsigned int, savedVolume_);
	CB_DEPIMPL(DevCDPlayList*, pPlayList_);

	stopPlaying();

	RICHARD_STREAM("Setting vol to saved volume " << savedVolume_ << std::endl);
	volume(savedVolume_);

    alDeleteSources(1, &source_);

	_DELETE( pPlayList_ );
	_DELETE( pImpl_ );

    TEST_INVARIANT;
}

void DevCD::update()
{
    if(needsUpdate_)
    {
        handleMessages(DevCD::SUCCESS, 0);
        needsUpdate_ = false;
    }
}

bool DevCD::isPlayingAudioCd() const
{
    CB_DEPIMPL(ALuint, source_);

    ALint sourceState;
    alGetSourcei(source_, AL_SOURCE_STATE, &sourceState);
    return sourceState == AL_PLAYING;
}

bool DevCD::supportsVolumeControl() const
{
	CB_DEPIMPL(bool, haveMixer_);
	return haveMixer_;
}

Volume DevCD::volume() const
{
	Volume percentageVolume = 0;
	if(supportsVolumeControl())
	{
		CB_DEPIMPL(unsigned int, savedVolume_);

		percentageVolume = savedVolume_;
		RICHARD_STREAM("Current percentage vol " << percentageVolume << std::endl);
	}
	return percentageVolume;
}

void DevCD::volume( Volume newLevel )
{
	if(supportsVolumeControl())
	{
		CB_DEPIMPL(unsigned int, savedVolume_);
		CB_DEPIMPL(ALuint, source_);

		if(newLevel > 100)
		{
			newLevel = 100;
		}
        savedVolume_ = newLevel;
        ALfloat fVol = (float)(savedVolume_) / 100.0f; // Maybe use log model instead of linear?
        alSourcef(source_, AL_GAIN, fVol);
		RICHARD_STREAM("NewVolume set to " << volume() << std::endl);
	}
}

DevCDTrackIndex DevCD::currentTrackIndex() const
{
	PRE( isPlayingAudioCd() );
    return trackPlaying_;
}

DevCDTrackIndex DevCD::numberOfTracks() const
{
	return 10+1; // Hardcoded number
}

Seconds DevCD::currentTrackLengthInSeconds() const
{
	PRE( isPlayingAudioCd() );
	return 0;
}

//TBD - Unable to implement through MCI
Seconds DevCD::currentTrackRunningTime() const
{
	PRE( isPlayingAudioCd() );
	ASSERT(false, "Function not implemented");
	return 0;
}

Seconds DevCD::currentTrackTimeRemaining() const
{
	PRE( isPlayingAudioCd() );
	ASSERT(false, "Function not implemented");
	return 0;
}

void DevCD::play()
{
    play(1);
}

void DevCD::playFrom( DevCDTrackIndex track )
{
	PRE( track >= 0 and track < numberOfTracks() );
    play(track);
}

static void eosCallback(void *unused, ALuint unused2)
{
    (void)unused;
    (void)unused2;
    DevCD::instance().needsUpdate_ = true;
}

void DevCD::play( DevCDTrackIndex track, bool repeat /* = false */ )
{
    CB_DEPIMPL(unsigned int, savedVolume_);

	PRE( track >= 0 and track < numberOfTracks() );

	trackPlaying_ = track;

    if(savedVolume_ <= 0) // Muted
        return;

    // Music streaming is not wired up on this platform (was `alure`).
    // Downloadable music content can be plugged in here later.

	if ( repeat )
		status_ = REPEAT;
	else
		status_ = SINGLE;
}

void DevCD::play( const DevCDPlayList& params )
{
	CB_DEPIMPL(DevCDPlayList*, pPlayList_);

	//Naughty and evil, replace with a copy construction
	*pPlayList_ = params;
	pPlayList_->reset();
	play(pPlayList_->firstTrack());

	status_ = PROGRAMMED;
}

void DevCD::stopPlaying()
{
    CB_DEPIMPL(ALuint, source_);
    alSourceStop(source_);
}

void DevCD::handleMessages( CDMessage message, unsigned int devID)
{
	CB_DEPIMPL(DevCDPlayList*, pPlayList_);
	CB_DEPIMPL(MexBasicRandom, randomGenerator_);
	CB_DEPIMPL(DevCDTrackIndex, randomStartTrack_);
	CB_DEPIMPL(DevCDTrackIndex, randomEndTrack_);

	switch(message)
	{
		case ABORT:
		break;

		case FAIL:
		break;

		case SUCCESS:
		{
			if(status_ == PROGRAMMED)
			{
				if(not pPlayList_->isFinished())
				{
					play(pPlayList_->nextTrack());
				}
			}
			else if ( status_ == REPEAT )
			{
				play( trackPlaying_, true );
			}
			else if ( status_ == RANDOM )
			{
				if ( randomStartTrack_ < numberOfTracks() )
				{
					// Make sure we're not asking it to randomise a number outside the range of tracks
					// on the CD.
					DevCDTrackIndex tmpEndTrack = std::min( numberOfTracks(), randomEndTrack_ );

					if ( randomStartTrack_ < tmpEndTrack )
					{
						// Make sure we don't play the same track twice (unless it is the only track)
						DevCDTrackIndex trackToPlay = trackPlaying_;
						while ( trackToPlay == trackPlaying_ and
								( trackToPlay != randomStartTrack_ or trackToPlay != tmpEndTrack ) )
						{
							trackToPlay = mexRandomInt( &randomGenerator_, randomStartTrack_, tmpEndTrack );
						}
						play( trackToPlay );
						status_ = RANDOM; // 'play' sets the status_ to SINGLE
					}
				}
			}
			break;
		}

		case SUPERSEDED:
		break;

		case UNKNOWN:
		break;

		default:
		break;
	}
}

bool DevCD::isAudioCDPresent()
{
	CB_DEPIMPL(unsigned int, savedVolume_);
    // If music is muted then just say no
    if(savedVolume_ <= 0)
        return false;
    return true;
}

ostream& operator <<( ostream& o, const DevCD& devCD)
{
	o << "Number of tracks " << devCD.numberOfTracks() << "\n"
	<< "Current Track " << devCD.currentTrackIndex() << "\n"
	<< "Track time " << devCD.currentTrackLengthInSeconds() << "\n"
	<< "Track running time " << devCD.currentTrackRunningTime() << "\n"
	<< "Track remaining time " << devCD.currentTrackTimeRemaining() << std::endl;

	return o;
}

void DevCD::randomPlay( DevCDTrackIndex startTrack, DevCDTrackIndex endTrack, DevCDTrackIndex firstTrack /*= -1*/ )
{
	CB_DEPIMPL(DevCDTrackIndex, randomStartTrack_);
	CB_DEPIMPL(DevCDTrackIndex, randomEndTrack_);
	CB_DEPIMPL(MexBasicRandom, randomGenerator_);

	PRE( startTrack >= 0 );
	PRE( startTrack <= endTrack );

    randomStartTrack_ = startTrack;
	randomEndTrack_ = endTrack + 1;

	if ( firstTrack != -1 )
	{
		play( firstTrack );
	}
	else
	{
		if ( randomStartTrack_ < numberOfTracks() )
		{
			// Make sure we're not asking it to randomise a number outside the range of tracks
			// on the CD.
			DevCDTrackIndex tmpEndTrack = std::min( numberOfTracks(), randomEndTrack_ );

			if ( randomStartTrack_ < tmpEndTrack )
			{
				DevCDTrackIndex trackToPlay = mexRandomInt( &randomGenerator_, randomStartTrack_, tmpEndTrack );
				play( trackToPlay );
			}
		}
	}

	status_ = RANDOM;
}

/* End CD.CPP *******************************************************/
