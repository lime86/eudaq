Scope Producer
=====

This producer enables to take waveform with a scope which is triggered by a TLU. 

### Supported Hardware

   * Keithley (Agilent) Infiniium 9000 and similar
   * foreseen LeCroy

### Description

Several data taking modes are supported. Partially the availability depends on the capabilities of the used scope or additional hardware.


### Requirements


### Usage

Set the following parameters in your config file:

```
[Producer.scope]

```

where the first parameter is the GPIO pin number to be used and the second is the waiting time before the action is executed.

### Status of the module

