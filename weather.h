#ifndef WEATHER_H
#define WEATHER_H

#define TEMPERATURE_SIZE     8
#define STATION_SIZE         3

struct range {
	uint16_t low;
	uint16_t high;
};

float forecast_for(char *, float, float);		// (station, min, max)
void hton_range(const struct range *, struct range *);	// (host, network)
void ntoh_range(const struct range *, struct range *);	// (network, host)

#endif
