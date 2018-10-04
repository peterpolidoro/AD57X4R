// ----------------------------------------------------------------------------
// AD57X4R.cpp
//
// Provides an SPI based interface to the AD57X4R
// Complete, Quad, 12-/14-/16-Bit, Serial Input,
// Unipolar/Bipolar Voltage Output DACs.
//
// Authors:
// Peter Polidoro peterpolidoro@gmail.com
// ----------------------------------------------------------------------------
#include "AD57X4R.h"


AD57X4R::AD57X4R()
{
}

AD57X4R::AD57X4R(const size_t chip_select_pin)
{
  setChipSelectPin(chip_select_pin);
}

void AD57X4R::setChipSelectPin(const size_t pin)
{
  pinMode(pin,OUTPUT);
  digitalWrite(pin,HIGH);
  cs_pin_ = pin;
}

void AD57X4R::setLoadDacPin(const size_t pin)
{
  pinMode(pin,OUTPUT);
  digitalWrite(pin,LOW);
  ldac_pin_ = pin;
}

void AD57X4R::setClearPin(const size_t pin)
{
  pinMode(pin,OUTPUT);
  digitalWrite(pin,HIGH);
  clr_pin_ = pin;
}

void AD57X4R::setup(const Resolution resolution,
                    const uint8_t chip_count)
{
  if ((chip_count >= CHIP_COUNT_MIN) && (chip_count <= CHIP_COUNT_MAX))
  {
    chip_count_ = chip_count;
  }
  else
  {
    chip_count_ = CHIP_COUNT_MIN;
  }
  resolution_ = resolution;
  unipolar_ = true;
  SPI.begin();
  powerUpAllDacs();
}

uint8_t AD57X4R::getChipCount()
{
  return chip_count_;
}

size_t AD57X4R::getChannelCount()
{
  return chip_count_*CHANNEL_COUNT_PER_CHIP;
}

void AD57X4R::setOutputRange(const size_t channel,
                             const Range range)
{
  uint8_t chip_index = channelToChipIndex(channel);
  uint8_t channel_address = channelToChannelAddress(channel);
  setOutputRangeToChip(chip_index,channel_address,range);
}

void AD57X4R::setOutputRangeAll(const Range range)
{
  uint8_t chip_index = CHIP_INDEX_ALL;
  uint8_t channel_address = CHANNEL_ADDRESS_ALL;
  setOutputRangeToChip(chip_index,channel_address,range);
}

long AD57X4R::getMinDacValue()
{
  long min_dac_value = 0;
  if (unipolar_)
  {
    min_dac_value = 0;
  }
  else
  {
    switch (resolution_)
    {
      case AD5724R:
        min_dac_value = -2048;
        break;
      case AD5734R:
        min_dac_value = -8192;
        break;
      case AD5754R:
        min_dac_value = -32768;
        break;
    }
  }
  return min_dac_value;
}

long AD57X4R::getMaxDacValue()
{
  long max_dac_value = 0;
  if (unipolar_)
  {
    switch (resolution_)
    {
      case AD5724R:
        max_dac_value = 4095;
        break;
      case AD5734R:
        max_dac_value = 16383;
        break;
      case AD5754R:
        max_dac_value = 65536;
        break;
    }
  }
  else
  {
    switch (resolution_)
    {
      case AD5724R:
        max_dac_value = 2047;
        break;
      case AD5734R:
        max_dac_value = 8191;
        break;
      case AD5754R:
        max_dac_value = 32767;
        break;
    }
  }
  return max_dac_value;
}

void AD57X4R::analogWrite(const size_t channel, const long value)
{
  uint8_t chip_index = channelToChipIndex(channel);
  uint8_t channel_address = channelToChannelAddress(channel);
  analogWriteToChip(chip_index,channel_address,value);
}

void AD57X4R::analogWriteAll(const long value)
{
  uint8_t chip_index = CHIP_INDEX_ALL;
  uint8_t channel_address = CHANNEL_ADDRESS_ALL;
  analogWriteToChip(chip_index,channel_address,value);
}

bool AD57X4R::channelPoweredUp(const size_t channel)
{
  uint8_t chip_index = channelToChipIndex(channel);
  uint8_t channel_address = channelToChannelAddress(channel);
  uint16_t data = readPowerControlRegister(chip_index);

  bool channel_powered_up = false;
  switch (channel_address)
  {
    case CHANNEL_ADDRESS_A:
      channel_powered_up = data & POWER_CONTROL_DAC_A;
      break;
    case CHANNEL_ADDRESS_B:
      channel_powered_up = data & POWER_CONTROL_DAC_B;
      break;
    case CHANNEL_ADDRESS_C:
      channel_powered_up = data & POWER_CONTROL_DAC_C;
      break;
    case CHANNEL_ADDRESS_D:
      channel_powered_up = data & POWER_CONTROL_DAC_D;
      break;
  }
  return channel_powered_up;
}

bool AD57X4R::referencePoweredUp(const uint8_t chip_index)
{
  uint16_t data = readPowerControlRegister(chip_index);

  bool reference_powered_up = data & POWER_CONTROL_REF;
  return reference_powered_up;
}

bool AD57X4R::thermalShutdown(const uint8_t chip_index)
{
  uint16_t data = readPowerControlRegister(chip_index);

  bool thermal_shutdown = data & POWER_CONTROL_THERMAL_SHUTDOWN;
  return thermal_shutdown;
}

bool AD57X4R::channelOverCurrent(const size_t channel)
{
  uint8_t chip_index = channelToChipIndex(channel);
  uint8_t channel_address = channelToChannelAddress(channel);
  uint16_t data = readPowerControlRegister(chip_index);

  bool channel_over_current = false;
  switch (channel_address)
  {
    case CHANNEL_ADDRESS_A:
      channel_over_current = data & POWER_CONTROL_OVERCURRENT_A;
      break;
    case CHANNEL_ADDRESS_B:
      channel_over_current = data & POWER_CONTROL_OVERCURRENT_B;
      break;
    case CHANNEL_ADDRESS_C:
      channel_over_current = data & POWER_CONTROL_OVERCURRENT_C;
      break;
    case CHANNEL_ADDRESS_D:
      channel_over_current = data & POWER_CONTROL_OVERCURRENT_D;
      break;
  }
  return channel_over_current;
}

// private
uint8_t AD57X4R::channelToChipIndex(const size_t channel)
{
  uint8_t chip_index = channel / CHANNEL_COUNT_PER_CHIP;
  return chip_index;
}

uint8_t AD57X4R::channelToChannelAddress(const size_t channel)
{
  uint8_t chip_channel = channel % CHANNEL_COUNT_PER_CHIP;
  uint8_t channel_address;
  switch (chip_channel)
  {
    case 0:
      channel_address = CHANNEL_ADDRESS_A;
      break;
    case 1:
      channel_address = CHANNEL_ADDRESS_B;
      break;
    case 2:
      channel_address = CHANNEL_ADDRESS_C;
      break;
    case 3:
      channel_address = CHANNEL_ADDRESS_D;
      break;
  }
  return channel_address;
}

void AD57X4R::csEnable()
{
  digitalWrite(cs_pin_,LOW);
}

void AD57X4R::csDisable()
{
  digitalWrite(cs_pin_,HIGH);
}
void AD57X4R::spiBeginTransaction()
{
  SPI.beginTransaction(SPISettings(SPI_CLOCK,SPI_BIT_ORDER,SPI_MODE));
  csEnable();
}

void AD57X4R::spiEndTransaction()
{
  csDisable();
  SPI.endTransaction();
}

void AD57X4R::writeMosiDatagramToChip(const int chip_index,
                                      const AD57X4R::Datagram mosi_datagram)
{
  spiBeginTransaction();
  for (int i=(DATAGRAM_SIZE - 1); i>=0; --i)
  {
    uint8_t byte_write = (mosi_datagram.uint32 >> (8*i)) & 0xff;
    SPI.transfer(byte_write);
  }
  spiEndTransaction();
}

AD57X4R::Datagram AD57X4R::readMisoDatagramFromChip(const int chip_index)
{
  Datagram mosi_datagram;
  mosi_datagram.uint32 = 0;
  mosi_datagram.fields.rw = RW_WRITE;
  mosi_datagram.fields.reg = REGISTER_CONTROL;
  mosi_datagram.fields.channel_address = CONTROL_ADDRESS_NOP;

  Datagram miso_datagram;
  miso_datagram.uint32 = 0;

  spiBeginTransaction();
  for (int i=(DATAGRAM_SIZE - 1); i>=0; --i)
  {
    uint8_t byte_write = (mosi_datagram.uint32 >> (8*i)) & 0xff;
    uint8_t byte_read = SPI.transfer(byte_write);
    miso_datagram.uint32 |= ((uint32_t)byte_read) << (8*i);
  }
  spiEndTransaction();

  return miso_datagram;
}

void AD57X4R::powerUpAllDacs()
{
  uint16_t data = (POWER_CONTROL_DAC_A |
                   POWER_CONTROL_DAC_B |
                   POWER_CONTROL_DAC_C |
                   POWER_CONTROL_DAC_D |
                   POWER_CONTROL_REF);

  Datagram mosi_datagram;
  mosi_datagram.uint32 = 0;
  mosi_datagram.fields.rw = RW_WRITE;
  mosi_datagram.fields.reg = REGISTER_POWER_CONTROL;
  mosi_datagram.fields.channel_address = CHANNEL_ADDRESS_POWER_CONTROL;
  mosi_datagram.fields.data = data;
  int chip_index = CHIP_INDEX_ALL;
  writeMosiDatagramToChip(chip_index,mosi_datagram);
}

void AD57X4R::setOutputRangeToChip(const int chip_index,
                                   const uint8_t channel_address,
                                   const Range range)
{
  uint16_t data;
  switch (range)
  {
    case UNIPOLAR_5V:
      unipolar_ = true;
      data = OUTPUT_RANGE_UNIPOLAR_5V;
      break;
    case UNIPOLAR_10V:
      unipolar_ = true;
      data = OUTPUT_RANGE_UNIPOLAR_10V;
      break;
    case UNIPOLAR_10V8:
      unipolar_ = true;
      data = OUTPUT_RANGE_UNIPOLAR_10V8;
      break;
    case BIPOLAR_5V:
      unipolar_ = false;
      data = OUTPUT_RANGE_BIPOLAR_5V;
      break;
    case BIPOLAR_10V:
      unipolar_ = false;
      data = OUTPUT_RANGE_BIPOLAR_10V;
      break;
    case BIPOLAR_10V8:
      unipolar_ = false;
      data = OUTPUT_RANGE_BIPOLAR_10V8;
      break;
    default:
      unipolar_ = true;
      data = OUTPUT_RANGE_UNIPOLAR_5V;
      break;
  }
  Datagram mosi_datagram;
  mosi_datagram.uint32 = 0;
  mosi_datagram.fields.rw = RW_WRITE;
  mosi_datagram.fields.reg = REGISTER_OUTPUT_RANGE;
  mosi_datagram.fields.channel_address = channel_address;
  mosi_datagram.fields.data = data;
  writeMosiDatagramToChip(chip_index,mosi_datagram);
}

void AD57X4R::analogWriteToChip(const int chip_index,
                                const uint8_t channel_address,
                                const long data)
{
  Datagram mosi_datagram;
  mosi_datagram.uint32 = 0;
  mosi_datagram.fields.rw = RW_WRITE;
  mosi_datagram.fields.reg = REGISTER_DAC;
  mosi_datagram.fields.channel_address = channel_address;
  switch (resolution_)
  {
    case AD5754R:
      mosi_datagram.fields.data = data;
      break;
    case AD5734R:
      mosi_datagram.fields.data = data << 2;
      break;
    case AD5724R:
      mosi_datagram.fields.data = data << 4;
      break;
  }
  writeMosiDatagramToChip(chip_index,mosi_datagram);
  load(chip_index);
}

void AD57X4R::load(const int chip_index)
{
  Datagram mosi_datagram;
  mosi_datagram.uint32 = 0;
  mosi_datagram.fields.rw = RW_WRITE;
  mosi_datagram.fields.reg = REGISTER_CONTROL;
  mosi_datagram.fields.channel_address = CONTROL_ADDRESS_LOAD;
  writeMosiDatagramToChip(chip_index,mosi_datagram);
}

uint16_t AD57X4R::readPowerControlRegister(const uint8_t chip_index)
{
  Datagram mosi_datagram;
  mosi_datagram.uint32 = 0;
  mosi_datagram.fields.rw = RW_READ;
  mosi_datagram.fields.reg = REGISTER_POWER_CONTROL;
  mosi_datagram.fields.channel_address = CHANNEL_ADDRESS_POWER_CONTROL;
  writeMosiDatagramToChip(chip_index,mosi_datagram);

  Datagram miso_datagram = readMisoDatagramFromChip(chip_index);
  return miso_datagram.fields.data;
}
