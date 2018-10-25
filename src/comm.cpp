#include "comm.h"

WaggleNode::WaggleNode(uint8_t CE_pin, uint8_t CS_pin)
    : radio(RF24(CE_pin, CS_pin)), network(RF24Network(radio)), mesh(RF24Mesh(radio, network)) {
}

void WaggleNode::begin() {
    begin(123);
}

void WaggleNode::begin(uint8_t radio_channel) {
    nodeid_t nodeid = get_sig_byte_();
    mesh.setNodeID(nodeID);
    Serial.print("NodeID: ");
    Serial.print(nodeid, HEX);

    Serial.println(F("Connecting to Mesh..."));
    mesh.begin(radio_channel, RF24_2MBPS);
}

void WaggleNode::update() {
    mesh.update();
    while (network.available()) {
        // Received now data
        RF24NetworkHeader header;
        size_t data_size = network.peek(header);
        byte *payload = new byte[data_size];
        network.read(header, payload, data_size);
        // TODO: Callback for received payload
        delete[] payload;
    }
}

/**
 * Retrieves ATMega328PB Device Unique ID for NodeID
 * Unique ID reside in bytes 14-23 (inclusive), take the last 4
 */
uint32_t get_sig_byte_() {
    uint8_t res[4];
    // Take bytes backward to match little endian
    for (int i = 0; i < 4; i++)
        res[23-i] = boot_signature_byte_get(i);
    return *(uint32_t*)res;
}

uint8_t WaggleNode::write_(void *payload, uint8_t ch, uint8_t len) {
    /* Returns:
    - 0: Normal
    - 1: Send failed, test OK
    - 2: Send failed, address is lost (will block)
    - 3: Invalid send call
    */
    auto my_id = mesh.getNodeID(mesh.mesh_address);
    if (my_id < 0 || my_id != nodeID) mesh.renewAddress();
    if (!mesh.write(payload, ch, len)) {
      // If a write fails, check connectivity to the mesh network
      if (!mesh.checkConnection()) {
        // refresh the network address
        Serial.println("Renewing Address");
        mesh.renewAddress();
        return 2;
      } else return 1;
    } else return 0;
}