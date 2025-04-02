# Final project proposal

- [x] I have reviewed the project guidelines.
- [x] I will be working alone on this project.
- [x] No significant portion of this project will be (or has been) used in other course work.

## Embedded System Description

This embedded system will simulate real-world usage of a microwave oven. It will implement a master/slave topology and be able to take cook power and cook time inputs from the user, as well as various pre-selected options (e.g. 50% power for one minute for popcorn), as well as a start/stop input. It will output the user's input selections in real time to an external LCD display, show the current cook power on an LED bar, and "beep" when finished via a piezo buzzer.

The cook power and cook time inputs will be implemented with a parallax membrane keypad. The "*" key allow the user to input cook power while the "#" key will allow them to input cook time. The 0-9 keys will represent the numeric input itself, and the A, B, C, and D keys will each represent a preselection of cook power and cook time.

The start/stop input will be received using a TTP223 capacitive touch sensor. If the user has correctly input a cook power and time, but not yet started the system, touching this sensor will start it, and if it is already running, a touch will stop it.

The LCD output will show the user's cook power and cook time selection in real time before the system has is in the running state, and display the cook time remaining (or "DONE") while the system is in the running state.

The current cook power (while the system is running) will be displayed using a 10 LED light bar. Each individual LED will represent one increment of the power level-- e.g. a power level of 1 would have only the first LED on, while a power level of five would have the first five on.

A piezo buzzer will be used to alert the user that the system has finished running (i.e. the cook time entered has expired). It will be inactive during the running state, and buzz loudly upon exiting the running state.

## Hardware Setup

Hardware:
- MSP430FR2355 Launchpad (master)
- MSP430FR2355 (slave)
- TTP223 capacitive touch sensor
- Parallax membrane keypad
- LCD display
- LED bar
- Piezo buzzer

!([Circuit Diagram](https://photos.app.goo.gl/9KWqJg1UNsiYzXE48))

## Software overview

The software for this system is simple and straightforward, if not trivial and compact. It essentially consists of two programs: one on the MSP430FR2355 Launchpad (the master), which is responsible for polling the keypad and touch sensor, then sending instructions via I2C to the other MSP430FR2355 (the slave) when the touch sensor is touched. The other resides on the slave, and is responsible for receiving I2C communication from the master, then sending the outputs corresponding to its instructions to the LCD, LED bar, and Piezo buzzer.

!([Software Diagram](https://photos.app.goo.gl/rPjJWunn4biQMQJS6))

## Testing Procedure

I will test and demonstrate this system by showing that each individual component works as intended, as well as showing an example real-world usage. This means that I'll show each input and output behaving as expected for the input I give it, and that they all work together as a cohesive whole for a set of inputs that a system like this might encounter in a real-world scenario.

## Prescaler

Desired Prescaler level: 

- [ ] 100%
- [ ] 95% 
- [x] 90% 
- [ ] 85% 
- [ ] 80% 
- [ ] 75% 

### Prescalar requirements

**Outline how you meet the requirements for your desired prescalar level**

**The inputs to the system will be:**
1.   Parallax membrane keypad
2.   Capacitive touch sensor

**The outputs of the system will be:**
1.   LCD display
2.   LED bar
3.   Piezo buzzer

**The project objective is**

Simulate real-world usage of a microwave oven, including the ability for the user to select cook power, cook time, and start and stop the system, while the outputs represent the system's current state.

**The new hardware or software modules are:**
1. Capacitive touch sensor


The Master will be responsible for:

Taking inputs from the user, and sending corresponding instructions to the slave.

The Slave(s) will be responsible for:

Receiving instructions from the master, and executing them to send the correct output to each external output device.

### Argument for Desired Prescaler

Given the guidelines requiring a 90% prescale project to have 5 combined inputs and outputs, have real-world applicability, and make use of new hardware and software, I think this proposal handily meets all requirements. It has two distinct inputs received by the master, three distinct outputs produced by the slave, and communication between the master and slave. Additionally, its function is very similar to what an actual microwave oven microcontroller's might be, so this represents a realistic real use case.