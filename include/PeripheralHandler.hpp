/** Class hierarchy to handle multipler peripherals
 * Handler Class Scope: 
 * 	Shall detect sensors and modules on the I2C bus.
 * 	Shall populate the object of each module and stores its information.
 *	Shall occasionally read sensor data and updates modules.
 *	Shall ocassionally re-detect sensors and modules on the I2C bus.
 *
 * Peripheral class scope:
 * 	Shall be a basic class for identifying I2C peripheral devices.
 *  Shall retain info [Address, type, status]
 *
 * Sensor subclass Scope:
 *	Shall retain info [Readings]
 *	Shall expose a single method for reading all values that can be used by the main class.
 *	Shall retain the values of last sensor read
 */

#pragma once
#include "AHT10.h"
#include "SparkFun_VEML6030_Ambient_Light_Sensor.h"

// Enum for decive types
enum PERIPHERAL_TYPE : uint8_t {
	SENSOR_UNKNOWN,
	SENSOR_TEMPERATURE_HUMIDITY,
	SENSOR_LIGHT,
	SENSOR_PRESSURE,
	SENSOR_GAS,
	SENSOR_CO2,
	SENSOR_TOUCH,
	MODULE_SOLAR,
	MODULE_LCD,
	MODULE_POV,
};

enum PERIPHERAL_STATUS : uint8_t {
	PERIPHERAL_STATUS_UNKNOWN,
	PERIPHERAL_STATUS_OFFLINE,
	PERIPHERAL_STATUS_ONLINE,
	PERIPHERAL_STATUS_ERROR,
	PERIPHERAL_STATUS_COMM_ERROR,
};

/** Base class for identifying I2C peripheral devices */
class Peripheral{
protected:
	uint8_t				_address[2];	// Address Accessible by derived classes
	PERIPHERAL_TYPE		_type;			// Device type (sensor, motule, etc)
	PERIPHERAL_STATUS	_status;		// Device status (online, offline, etc)

public:
	Peripheral(PERIPHERAL_TYPE type, uint8_t address[2], PERIPHERAL_STATUS status) {
		_address[0] = address[0];
		_address[1] = address[1];
		_type = type;
		_status = status;
	}
	virtual ~Peripheral() = default;

	const uint8_t* getAddress() const { return _address; }
	uint8_t getType()	const { return _type; }
	uint8_t getStatus()	const { return _status; }
	void setCommError()	{ _status = PERIPHERAL_STATUS_COMM_ERROR; }
	void setError()		{ _status = PERIPHERAL_STATUS_ERROR; }
	void resetError()   { _status = PERIPHERAL_STATUS_ONLINE; }
}

/** Derived class for Temperature-Humidity Sensor */
class Sensor_TH : public Peripheral {
private:
    AHT10* sensor_library;
    float sensor_temperature;
    float sensor_humidity;

public:
    Sensor_TH(uint8_t new_address[2], AHT10* lib) : Peripheral(SENSOR_TEMPERATURE_HUMIDITY, new_address, PERIPHERAL_STATUS_ONLINE)
    {
        sensor_library = lib; // Initialize member variable in the body
        sensor_temperature = 0.0f; // Initialize temperature
        sensor_humidity = 0.0f; // Initialize humidity
    }
    ~Sensor_TH() override = default;

    /**Get the sensor readings via I2C interface
     * Values are stored into the object's variables
     * 
     * @return 1 if success, 0 if error
     */
    bool readSensorData(){
		sensor_temperature = sensor_library->readTemperature();
		sensor_humidity = sensor_library->readHumidity();
        if(sensor_temperature == 0xFF || sensor_humidity == 0xFF){
			sensor_temperature = 0.0f;	// Reset temperature
        	sensor_humidity = 0.0f;		// Reset humidity
            setError();
            return false;
        }
        resetError();
        return true;
    }
    float getTemperature() const { return sensor_temperature; }
    float getHumidity() const { return sensor_humidity; }
};

/** Derived class for Ambient Light Sensor */
class Sensor_AL : public Peripheral {
private:
    SparkFun_Ambient_Light* sensor_library;
    float sensor_lux;

public:
    Sensor_AL(uint8_t new_address[2], SparkFun_Ambient_Light* lib) : Peripheral(SENSOR_LIGHT, new_address, PERIPHERAL_STATUS_ONLINE)
    {
        sensor_library = lib;	// Initialize member variable in the body
        sensor_lux = 0.0f;		// Initialize luminance
    }
    ~Sensor_AL() override = default;

    /**Get the sensor readings via I2C interface
     * Values are stored into the object's variables
     * 
     * @return 1 if success, 0 if error
     */
    bool readSensorData(){
		sensor_lux = static_cast<float>(sensor_library->readLight());
        if (sensor_lux < 0.0f) {
			sensor_lux = 0.0f;
            setError();
            return false;
        }
        resetError();
        return true;
    }
    float getLight() const { return sensor_lux; }
};