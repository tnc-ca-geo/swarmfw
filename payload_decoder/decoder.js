/**
    * A decoder for the SWARM firmware in this repository, implements
    * https://github.com/tnc-ca-geo/swarmfw/README.md
*/

/**
    * Helper functions from here on. Some are separated for unit testing and
    * code readability in particulat if conversions require a few steps of
    * akin transformations.
*/

const meterTeras12 = ['calibratedCountsVWC', 'soilTemp_C', 'conductivity'];
const tncSpecificLookup = {
  '50': ['pressure', 'waterTmp'],
  '51': [
    'solarFluxDensity_W_per_m2', 'precip_mm', 'lightng_ct',
    'lightngDist_km', 'windSpeed_m_per_s', 'windDir_deg',
    'maxWindSp_m_per_s', 'airTmp_c', 'vaporPr_kPa',
    'barometricPr_kPa', 'relHumidity_0_1', 'humSensorTemp_C',
    'tiltNS_deg', 'tiltWE_deg', 'compass_unused',
    'windSpeedN_m_per_s', 'windSpeedE_m_per_s', 'windSpeedMax_per_s'],
  '52': ['leafWetness_percent'],
  '53': meterTeras12,
  '54': meterTeras12,
  '55': meterTeras12,
};

/**
    * Split a SDI-12 line separated by '-' and '+', maintain signage
    * @param {String} sdi12Line
    * @return {Array.<Number>}
*/
const sdi12Parse = (sdi12Line) => sdi12Line.split(/(?=[-+])/g).map(Number);

/**
    * Convert rxTime string into a Date object that matches the correct UTC.
    * Is there really no better way to interpret the time string as UTC
    * while parsing?
    @param {String} timeString hiveRxTime String when SWARM Server received
      message
    @return {Object}
*/
const rxTimeToUtc = (timeString) => {
  const wrongDate = new Date(timeString);
  return new Date(wrongDate - wrongDate.getTimezoneOffset() * 60000);
};

/**
    Convert payload time epoch to Date object.
    @param {Number} epoch epoch in seconds
    @return {Object}
*/
const payloadTimeToUtc = (epoch) => new Date(epoch * 1000);

/** end helper functions */

/**
    * A sensor parser that creates fields_{n} as needed
    * @param {String} sdi12Line
    * @param {Array.<string>} lookup A list of field names to use
    * @return {Object}
*/
const genericSensor = (sdi12Line, lookup=[]) => {
  const values = sdi12Parse(sdi12Line);
  return values.reduce(
      (o, item, idx) => (
        {...o, [lookup[idx] || 'field_' + idx]: item}), {});
};

/**
  * Parsing a 'CS' message specific to Falk Schuetzenmeister's FW in this repo.
  * @param {Array.<String>} fields An array of CSV pieces
  * @return {Array.<Object>}
*/
const csMessageParser = (fields) => {
  const ret = {};
  for (let i=4; i<fields.length; i=i+2) {
    ret[fields[i]] = genericSensor(fields[i+1], tncSpecificLookup[fields[i]]);
  }
  return ret;
};

/**
    * The decoder function. This function is kept generic, TNC or CHI specific
    * conventions are implemented in tncSpecificLookup
    * @param {String} payload The webhook payload
    * @return {Object}
*/
const decoder = (payload) => {
  // json load message
  let message = {};
  const ret = {};
  try {
    message = JSON.parse(payload);
  } catch (e) {
    // TODO: refine, JSON.parse throws SyntaxError but other things could too
    if (e instanceof SyntaxError) {
      return {'error': 'JSON parser error'};
    }
  }

  // carry over some metadata
  ret.swarm = {
    application: message.userApplicationId,
    device: message.deviceId,
    organization: message.organizationId,
    rxTime: rxTimeToUtc(message.hiveRxTime),
  };

  // Currently only parse data for application = 0 since other applications
  // could use entirely different payload formats in the future.
  if (ret.swarm.application !== 0) return ret;

  // parse the base64 payload
  payload = atob(message.data);
  // interpret payload as CSV
  fields = payload.split(',');
  // index since last restart of the device
  ret.user = {
    messagesSinceRestart: Number(fields[0]),
    payloadTime: payloadTimeToUtc(fields[1]),
    batteryVoltage: Number(fields[2]),
    messageType: fields[3],
  };

  // Currently only parse 'CS' messages since other types are not spec'ed yet
  if (ret.user.messageType !== 'SC') return ret;

  // add sensor readings
  ret.user.sensors = csMessageParser(fields);

  return ret;
};

module.exports = {
  payloadTimeToUtc, rxTimeToUtc, sdi12Parse,
  genericSensor,
  csMessageParser,
  decoder,
};
