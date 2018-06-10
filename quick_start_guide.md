# Quick Start Guide

## Setup

Communication is done over UART from a master microcontroller to an arbitrary number of avocados. Plug UART cable into your microcontroller and the avocado.

The first thing you will want to do is set your avocado's address. Use the provided `setAddress` function of the user library or construct the appropriate message following the provided protocol.

## Communication

See [protocol.md](./protocol.md) for more documentation on the structure of messages. The [C library](https://github.com/avocado-actuator/c_library) provides wrapper functions for every action you can take with your Avocado. Currently it is written for TI microcontrollers, in the future we may expand to support other architectures.
