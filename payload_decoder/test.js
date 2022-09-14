const decoder = require('./decoder');


const wellTestPayload =
  '{"data":"MDAwNTI5LDE2NjMwMjM2MDcsMy44NSxTQyw1MCwrMTMuMzA0NSsxNi4yNzE5",' +
  '"decoded":"000529,1663023607,3.85,SC,50,+13.3045+16.2719","deviceId":7328,' +
  '"deviceType":1,"hiveRxTime":"2022-09-13T00:16:09","len":45,' +
  '"organizationId":2151,"packetId":33910682,"status":0,"userApplicationId":0}';
const multipleSensorPayLoad =
  '{"data":"MDAwNzk3LDE2Mzk1MjY0MDEsMy43MyxTQyw1MSwrOTMrMC4zNDArMCswKzIuNDQ' +
  'rMzA2LjcrNy45OCs4LjcrMC45NysxMDAuMDcrMC44NjArOC43LTAuMisxLjArMCsxLjQ2LTEu' +
  'OTYrNy45OCw1MiwrMC4wMCw1MywrMjQ1MS45MCsxNS42KzM0Mg==","decoded":"000797,1' +
  '639526401,3.73,SC,51,+93+0.340+0+0+2.44+306.7+7.98+8.7+0.97+100.07+0.860+8' +
  '.7-0.2+1.0+0+1.46-1.96+7.98,52,+0.00,53,+2451.90+15.6+342","deviceId":' +
  '3418,"deviceType":1,"hiveRxTime":"2021-12-15T04:36:42","len":139,' +
  '"organizationId":2151,"packetId":17466188,"status":0,"userApplicationId":0}';


test('test csMessageParser with generic parser', () => {
  const testArray = [
    '000529', 1663023607, 3.85, 'SC', 65, '+13.3045+16.2719', 66, '+19-20'];
  expect(decoder.csMessageParser(testArray)).toStrictEqual({
    65: {'field_0': 13.3045, 'field_1': 16.2719},
    66: {'field_0': 19, 'field_1': -20}});
});


test('test sdi12Parse', () => {
  const test = '+12-12+12.1-3';
  expect(decoder.sdi12Parse(test)).toStrictEqual([12, -12, 12.1, -3]);
});


test('testGenericSensor', () => {
  const test = '+12-12+12.1-3';
  const expected = {
    'field_0': 12, 'field_1': -12, 'field_2': 12.1, 'field_3': -3};
  expect(decoder.genericSensor(test)).toStrictEqual(expected);
});


test('hiveRxTime conversion', () => {
  // rx time should be 1663028169 or 2022-09-13 00:16:09 UTC
  expect(
      decoder.rxTimeToUtc('2022-09-13T00:16:09') - 1663028169000).toBe(0);
});


test('payload time conversion', () => {
  // payload time should be 1663023607 or 2022-09-12 23:00:07 UTC
  expect(decoder.payloadTimeToUtc(1663023607) - 1663023607000).toBe(0);
});


test('well decoder', () => {
  // rx time should be 1663028169 or 2022-09-13 00:16:09 UTC
  // payload time should be 1663023607 or 2022-09-12 23:00:07 UTC
  expect(decoder.decoder(wellTestPayload)).toStrictEqual({
    swarm: {
      application: 0,
      device: 7328,
      organization: 2151,
      rxTime: new Date('2022-09-13T00:16:09.000Z'),
    },
    user: {
      messagesSinceRestart: 529,
      payloadTime: new Date('2022-09-12T23:00:07.000Z'),
      batteryVoltage: 3.85,
      messageType: 'SC',
      sensors: {
        '50': {
          'pressure': 13.3045,
          'waterTmp': 16.2719,
        },
      },
    },
  });
});

test('multipleSensorPayload', () => {
  const expected = {
    'swarm': {
      'application': 0,
      'device': 3418,
      'organization': 2151,
      'rxTime': new Date('2021-12-15T04:36:42.000Z')},
    'user': {
      'batteryVoltage': 3.73,
      'messageType': 'SC',
      'messagesSinceRestart': 797,
      'payloadTime': new Date('2021-12-15T00:00:01.000Z'),
      'sensors': {
        '51': {
          'airTmp_c': 8.7,
          'barometricPr_kPa': 100.07,
          'compass_unused': 0,
          'humSensorTemp_C': 8.7,
          'lightngDist_km': 0,
          'lightng_ct': 0,
          'maxWindSp_m_per_s': 7.98,
          'precip_mm': 0.34,
          'relHumidity_0_1': 0.86,
          'solarFluxDensity_W_per_m2': 93,
          'tiltNS_deg': -0.2,
          'tiltWE_deg': 1,
          'vaporPr_kPa': 0.97,
          'windDir_deg': 306.7,
          'windSpeedMax_per_s': 7.98,
          'windSpeedE_m_per_s': -1.96,
          'windSpeedN_m_per_s': 1.46,
          'windSpeed_m_per_s': 2.44},
        '52': {
          'leafWetness_percent': 0},
        '53': {
          'calibratedCountsVWC': 2451.9,
          'conductivity': 342,
          'soilTemp_C': 15.6}}}};
  expect(decoder.decoder(multipleSensorPayLoad)).toStrictEqual(expected);
});


test('nonsensical input', () => {
  expect(decoder.decoder('quatsch')).toStrictEqual({
    'error': 'JSON parser error',
  });
});
