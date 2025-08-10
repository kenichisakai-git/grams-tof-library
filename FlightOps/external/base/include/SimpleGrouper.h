#ifndef __PETSYS_SIMPLE_GROUPER_HPP__DEFINED__
#define __PETSYS_SIMPLE_GROUPER_HPP__DEFINED__

#include <SystemConfig.h>
#include <UnorderedEventHandler.h>
#include <Event.h>
#include <Instrumentation.h>

namespace PETSYS {
	
class SimpleGrouper : public UnorderedEventHandler<Hit, GammaPhoton> {
public:
	SimpleGrouper(SystemConfig *systemConfig, EventSink<GammaPhoton> *sink);
	~SimpleGrouper();
	
	virtual void report();
	
protected:
	virtual EventBuffer<GammaPhoton> * handleEvents(EventBuffer<Hit> *inBuffer);
		
private:
	SystemConfig *systemConfig;
	
	u_int64_t nHitsReceived;
	u_int64_t nHitsReceivedValid;
	u_int64_t nPhotonsFound;
	u_int64_t nPhotonsHits[GammaPhoton::maxHits];
	u_int64_t nPhotonsHitsOverflow;
	u_int64_t nPhotonsHitsUnderflow;
	u_int64_t nPhotonsLowEnergy;
	u_int64_t nPhotonsHighEnergy;
	u_int64_t nPhotonsPassed;
};

}
#endif // __PETSYS_SIMPLE_GROUPER_HPP__DEFINED__
