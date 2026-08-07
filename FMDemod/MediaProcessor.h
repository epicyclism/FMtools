//
//	Wraps the media file.
//
//  This version assumes wavs and depends on winmm.dll for the dirty stuff.
//
//

#include "WavFileWrap.h"
#include "HRException.h"

class MediaProcessor
{
private :
	WaveFile wav_ ;

public :
	MediaProcessor ()
	{
	}

	void Open ( LPCTSTR sFile )
	{
		HRESULT hr = wav_.Open ( sFile, 0, WAVEFILE_READ ) ;
		if ( FAILED ( hr ))
			throw HRException ( hr, "Open failed" ) ;
		WAVEFORMATEX* pWFex = wav_.GetFormat () ;
		if( pWFex->nChannels > 2 )
			throw HRException ( E_FAIL, "Only mono or stereo wavs are supported." ) ;
	}

	// extract the entire audio into C
	// C assumed to be a container of some sort of floating point type. For now.
	// values are normalised to between -1.0 and 1.0
	//
	// proc flags = 0, convert to mono, 1 = use left, 2 = use right
	// if input is mono then ignore
	//
	template <typename C> void Read ( C& out, unsigned nProcFlags )
	{
		WAVEFORMATEX* pWFex = wav_.GetFormat () ;
		size_t depth = pWFex->wBitsPerSample ;
		typename C::value_type divisor ;
		// should do this programmatically...
		DWORD nbytespersample ;
		unsigned msb ;
		switch ( depth )
		{
		case 8 :
			divisor = 0x80 ;
			msb = 0xffffff80 ;
			nbytespersample = 1 ;
			break ;
		case 16 :
			divisor = 0x8000 ;
			msb = 0xffff8000 ;
			nbytespersample = 2 ;
			break ;
		case 24 :
			divisor = 0x800000 ;
			msb = 0xff800000 ;
			nbytespersample = 3 ;
			break ;
		case 32 :
			divisor = 0x80000000 ; // yeah right
			msb = 0x80000000 ;
			nbytespersample = 4 ;
			break ;
		default :
			divisor = 1 ;
			msb = 0x01 ;
			nbytespersample = 0 ;
			break ;
		}
		if ( nbytespersample == 0 )
			throw HRException ( E_FAIL, "Invalid bits per sample in wav file." ) ;

		if ( nProcFlags > 2 )
			throw HRException ( E_FAIL, "Invalid channel flags." ) ;

		DWORD dwRead ;	
		DWORD sz = wav_.GetSize () ;
		// read it all
		if ( pWFex->nChannels == 1 )
		{
			out.reserve ( sz ) ;
			while ( 1 )
			{
				unsigned sample = 0 ;
				HRESULT hr = wav_.Read ( reinterpret_cast<BYTE*>(&sample), nbytespersample, &dwRead ) ;
				if ( FAILED ( hr ))
				{
					throw HRException ( hr, "Failed to read wav file!" ) ;
				}
				if ( dwRead != nbytespersample )
					break ;
				if ( sample & msb ) // manual sign extend
					sample |= msb ;

				typename C::value_type fsample = static_cast< C::value_type> ( int( sample )) ;

				out.push_back ( fsample / divisor ) ;
			}
		}
		else // 2 channels
		{
			out.reserve ( sz / 2 ) ;
			while ( 1 )
			{
				unsigned sampleL = 0 ;
				unsigned sampleR = 0 ;
				HRESULT hr = wav_.Read ( reinterpret_cast<BYTE*>(&sampleL), nbytespersample, &dwRead ) ;
				if ( FAILED ( hr ))
				{
					throw HRException ( hr, "Failed to read wav file!" ) ;
				}
				if ( dwRead != nbytespersample )
					break ;
				if ( sampleL & msb ) // manual sign extend
					sampleL |= msb ;
				hr = wav_.Read ( reinterpret_cast<BYTE*>(&sampleR), nbytespersample, &dwRead ) ;
				if ( FAILED ( hr ))
				{
					throw HRException ( hr, "Failed to read wav file!" ) ;
				}
				if ( dwRead != nbytespersample )
					break ;
				if ( sampleR & msb ) // manual sign extend
					sampleR |= msb ;

				typename C::value_type fsample ;
				switch ( nProcFlags )
				{
				case 0 : // mono
					{
						typename C::value_type fL = static_cast< C::value_type> ( int( sampleL )) ;
						typename C::value_type fR = static_cast< C::value_type> ( int( sampleR )) ;
						fsample = ( fL + fR ) / typename C::value_type( 2.0 ) ;
					}
					break ;
				case 1 : // left
					fsample = static_cast< C::value_type> ( int( sampleL )) ;
					break ;
				case 2 : // right
					fsample = static_cast< C::value_type> ( int( sampleR )) ;
					break ;
				}

				out.push_back ( fsample / divisor ) ;
			}

		}
		// done!
	}

	// assume stereo and extract into a container of complex<T>
	//
	template <typename C> void ReadComplex ( C& out )
	{
		WAVEFORMATEX* pWFex = wav_.GetFormat () ;
		size_t depth = pWFex->wBitsPerSample ;
		typename C::value_type::value_type divisor ;
		// should do this programmatically...
		DWORD nbytespersample ;
		unsigned msb ;
		switch ( depth )
		{
		case 8 :
			divisor = 0x80 ;
			msb = 0xffffff80 ;
			nbytespersample = 1 ;
			break ;
		case 16 :
			divisor = 0x8000 ;
			msb = 0xffff8000 ;
			nbytespersample = 2 ;
			break ;
		case 24 :
			divisor = 0x800000 ;
			msb = 0xff800000 ;
			nbytespersample = 3 ;
			break ;
		case 32 :
			divisor = 0x80000000 ; // yeah right
			msb = 0x80000000 ;
			nbytespersample = 4 ;
			break ;
		default :
			divisor = 1 ;
			msb = 0x01 ;
			nbytespersample = 0 ;
			break ;
		}
		if ( nbytespersample == 0 )
			throw HRException ( E_FAIL, "Invalid bits per sample in wav file." ) ;

		if ( pWFex->nChannels == 1 )
			throw HRException ( E_FAIL, "Invalid operation on mono file." ) ;

		DWORD dwRead ;	
		DWORD sz = wav_.GetSize () ;
		out.reserve ( sz / 2 ) ;
		while ( 1 )
		{
			unsigned sampleL = 0 ;
			unsigned sampleR = 0 ;
			HRESULT hr = wav_.Read ( reinterpret_cast<BYTE*>(&sampleL), nbytespersample, &dwRead ) ;
			if ( FAILED ( hr ))
			{
				throw HRException ( hr, "Failed to read wav file!" ) ;
			}
			if ( dwRead != nbytespersample )
				break ;
			if ( sampleL & msb ) // manual sign extend
				sampleL |= msb ;
			hr = wav_.Read ( reinterpret_cast<BYTE*>(&sampleR), nbytespersample, &dwRead ) ;
			if ( FAILED ( hr ))
			{
				throw HRException ( hr, "Failed to read wav file!" ) ;
			}
			if ( dwRead != nbytespersample )
				break ;
			if ( sampleR & msb ) // manual sign extend
				sampleR |= msb ;

			typename C::value_type fsample ( static_cast< C::value_type::value_type> ( int( sampleR )), static_cast< C::value_type::value_type> ( int( sampleL ))) ;

			out.push_back ( fsample / divisor ) ;
		}

		// done!
	}

	size_t SampleRate ()
	{
		WAVEFORMATEX* pWFex = wav_.GetFormat () ;

		return pWFex->nSamplesPerSec ;
	}
} ;