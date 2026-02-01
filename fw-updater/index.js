import {SerialPort} from 'serialport';

const PACKET_DATA_BYTES         = 16;
const PACKET_LENGTH_BYTES       = 1;
const PACKET_CRC_BYTES          = 1;
const PACKET_CRC_INDEX          = PACKET_LENGTH_BYTES + PACKET_DATA_BYTES;
const PACKET_LENGTH             = PACKET_LENGTH_BYTES + PACKET_DATA_BYTES + PACKET_CRC_BYTES;

const PACKET_ACK_DATA0          = 0x15;
const PACKET_RETX_DATA0         = 0x19;

const serialPath                = "/dev/ttyUSB0";
const baudRate                  = 115200

const crc8 = (data) => {
    let crc = 0;

    for (const byte of data) {
        crc = (crc ^ byte) & 0xff;
        for (let i = 0; i < 8; i++) {
            if (crc & 0x80) {
                crc = ((crc << 1) ^ 0x07) & 0xff;
            } else {
                crc = (crc << 1) & 0xff;
            }
        }
    }

    return crc;
};

const delay = (ms) => new Promise((r) => setTimeout(r, ms));

class Packet {
    length;
    data;
    crc;

    static retx = new Packet(1, Buffer.from([PACKET_RETX_DATA0])).toBuffer();
    static ack = new Packet(1, Buffer.from([PACKET_ACK_DATA0])).toBuffer();

    constructor(length, data, crc) {
        this.length = length;
        this.data = data;

        const bytesToPad = PACKET_DATA_BYTES - this.data.length;
        const padding = Buffer.alloc(bytesToPad).fill(0xff);
        this.data = Buffer.concat([this.data, padding]);

        if (typeof crc == 'undefined')
            this.crc = this.computeCrc();
        else
            this.crc = crc;
    }

    computeCrc() {
        return crc8(Buffer.concat([Buffer.from([this.length]), this.data]));
    }

    toBuffer() {
        return Buffer.concat([ Buffer.from([this.length]), this.data, Buffer.from([this.crc]) ]);
    }

    isSingleBytePacket(byte) {
        if (this.length !== 1) return false;
        if (this.data[0] !== byte) return false;
        for (let i = 1; i < PACKET_DATA_BYTES; i++){
            if (this.data[i] !== 0xff) return false;
        }
        return true;
    }

    isAck() {
        return this.isSingleBytePacket(PACKET_ACK_DATA0);
    }

    isRetx() {
        return this.isSingleBytePacket(PACKET_RETX_DATA0);
    }
}

const uart = new SerialPort({ path: serialPath, baudRate });

let packets = [];

let lastPacket = Packet.ack;

const writePacket = (packet) => {
    uart.write(packet);
    lastPacket = packet;
}

let rxBuffer = Buffer.from([]);
const consumeFromBuffer = (n) => {
    const consumed = rxBuffer.slice(0, n);
    rxBuffer = rxBuffer.slice(n);
    return consumed;
}

uart.on('data', data => {
    console.log(`Received ${data.length} bytes through uart`);
    rxBuffer = Buffer.concat([rxBuffer, data]);

    if (rxBuffer.length >= PACKET_LENGTH) {
        console.log(`Building a packet`);
        const raw = consumeFromBuffer(PACKET_LENGTH);
        const packet = new Packet(raw[0], raw.slice(1, 1+PACKET_DATA_BYTES), raw[PACKET_CRC_INDEX]);
        const computedCrc = packet.computeCrc();

        if (packet.crc !== computedCrc) {
            console.log(`CRC failed, computed 0x${computedCrc.toString(16)}, got 0x${packet.crc.toString(16)}`);
            writePacket(Packet.retx);
            return;
        }

        if (packet.isRetx()) {
            console.log(`Re-transmitting last packet`);
            writePacket(lastPacket);
            return;
        }

        if (packet.isAck()) {
            console.log(`Ack`);
            return;
        }

        console.log(`Storing packet and ack'ing`);
        packets.push(packet);
        writePacket(Packet.ack);
    }
});

const waitForPacket = async () => {
    while (packets.length < 1) {
        await delay(1);
    }
    const packet = packets[0];
    packets = packets.slice(1);
    return packet;
}

const main = async () => {
    console.log('Waiting for packet...');
    const packet = await waitForPacket();
    console.log(packet);

    const packetToSend = new Packet(4, Buffer.from([8, 8, 8, 8]));

    uart.write(packetToSend.toBuffer());
}

main();