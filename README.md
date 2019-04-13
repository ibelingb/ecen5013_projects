# ecen5013_project
ECEN 5013 Project1 and Project2 Repository

This repository contains the source code for Project 1 (BBG based I2C sampling application) and Project 2 (TI TIVA and BBG application).

## Project 2
This project seeks to develop an automated watering system for a houseplant or garden. The BBG device will act as the command and data handling device (controlNode) for the system while the TI TIVA device will act as a remote sensing and actuating device (remoteNode). The two Nodes will communicate via a TCP/IP network interface to pass data and commands back and forth.

The remoteNode will capture data from a soil moisture sensor and a light sensor to determine a plant's soil moisture level and track day/night cycles. The user will be able to provide a watering schedule for how often the plant should be watered, and what amount of water should be provided to the plant. Sensor data is sent back to the controlNode which will log the data and take actions based on that received data.

The user interface will be provided via a serial console connected to the BBG controlNode. A numbered menu will be provided to display actions and data requests the user can make. This will include updating the watering schedule, watering the plant, reporting sensor data, etc.
