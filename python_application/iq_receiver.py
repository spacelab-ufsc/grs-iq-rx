import zmq
import numpy as np
import argparse
from pluto import Pluto
from usrp import USRP

BUFFER_SIZE_BYTES = 2097152 # 2 MB

class IQ_Receiver:
    def __init__(self):
        parser = argparse.ArgumentParser(description="GRS IQ Receiver")
        parser.add_argument("-s", "--sdr", type=str, required=True, choices=["pluto", "usrp"])
        parser.add_argument("-f", "--frequency", type=lambda x: int(float(x)), required=True)
        parser.add_argument("-r", "--sample_rate", type=lambda x: int(float(x)), required=True)
        parser.add_argument("-g", "--gain", type=float, required=True)
        config = parser.parse_args()
        
        self.context = zmq.Context()

        self.pub_socket = self.context.socket(zmq.PUB)
        self.pub_socket.setsockopt(zmq.SNDHWM, 10)
        self.pub_socket.setsockopt(zmq.RCVHWM, 10)
        self.pub_socket.bind("tcp://*:5556")

        self.cmd_socket = self.context.socket(zmq.SUB)
        self.cmd_socket.connect("tcp://localhost:5557")
        self.cmd_socket.setsockopt_string(zmq.SUBSCRIBE, "tune")

        sdr_type = config.sdr
        if sdr_type.lower() == "pluto":
            self.sdr = Pluto(sample_rate=config.sample_rate, gain=config.gain)
        elif sdr_type.lower() == "usrp":
            self.sdr = USRP(sample_rate=config.sample_rate, gain=config.gain)
        else:
            raise ValueError(f"Invalid SDR chosen: {sdr_type}\nMust be either usrp or pluto")
        self.center_freq = config.frequency
        self.sdr.rx_lo = self.center_freq

        self._iq_buffer = []
        self._buffered_bytes = 0

    def _handle_freq_synth(self):
        try:
            topic, new_freq_bytes = self.cmd_socket.recv_multipart(flags=zmq.NOBLOCK)
            new_freq = float(new_freq_bytes.decode())
            print(f"Retuning to {new_freq} Hz")
            self.sdr.rx_lo = int(new_freq)
        except zmq.Again:
            pass

    def run(self):
        print("IQ Receiver running")
        while True:
            self._handle_freq_synth()

            iq_samples = self.sdr.rx()
            chunk = iq_samples.astype(np.complex64)
            chunk_bytes = chunk.tobytes()

            self._iq_buffer.append(chunk_bytes)
            self._buffered_bytes += len(chunk_bytes)

            if self._buffered_bytes >= BUFFER_SIZE_BYTES:
                message = b"".join(self._iq_buffer)
                self.pub_socket.send_multipart([b"iq_data", message])

                self._iq_buffer = []
                self._buffered_bytes = 0