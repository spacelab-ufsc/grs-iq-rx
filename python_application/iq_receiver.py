import zmq
import numpy as np
from pluto import Pluto
from usrp import USRP

BUFFER_SIZE_BYTES = 2097152 # 2 MB

class IQ_Receiver:
    def __init__(self, config):
        self.context = zmq.Context()

        self.pub_socket = self.context.socket(zmq.PUB)
        self.pub_socket.setsockopt(zmq.SNDHWM, 10)   # Max of 10 messages queued on sender (2 MB each)
        self.pub_socket.setsockopt(zmq.RCVHWM, 10)   # Max of 10 messages queued in receiver
        self.pub_socket.bind("tcp://*:5556")

        self.cmd_socket = self.context.socket(zmq.SUB)
        self.cmd_socket.connect("tcp://localhost:5557")
        self.cmd_socket.setsockopt_string(zmq.SUBSCRIBE, "tune")

        sdr_type = config["SDR"]
        if sdr_type.lower() == "pluto":
            self.sdr = Pluto(sample_rate=config["sample_rate"], gain=config["gain"])
            self.center_freq = config["freq"]
            self.sdr.rx_lo = int(self.center_freq)
        elif sdr_type.lower() == "usrp":
            self.sdr = USRP(sample_rate=config["sample_rate"], gain=config["gain"])
            self.center_freq = config["freq"]
            self.sdr.rx_lo = int(self.center_freq)
        else:
            raise ValueError(f"Invalid SDR chosen: {sdr_type}")

        self._iq_buffer = []
        self._buffered_bytes = 0

    def _handle_commands(self):
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
            self._handle_commands()

            iq_samples = self.sdr.rx()  # Returns complex64 np array
            chunk = iq_samples.astype(np.complex64)
            chunk_bytes = chunk.tobytes()

            self._iq_buffer.append(chunk_bytes)
            self._buffered_bytes += len(chunk_bytes)

            if self._buffered_bytes >= BUFFER_SIZE_BYTES:
                message = b"".join(self._iq_buffer)
                self.pub_socket.send_multipart([b"iq_data", message])

                # Reset buffer
                self._iq_buffer = []
                self._buffered_bytes = 0

if __name__ == "__main__":
    conf = {"sample_rate": 1e6, "gain": 30, "freq": 420.9e6, "SDR": "pluto"}
    rx = IQ_Receiver(conf)
    rx.run()