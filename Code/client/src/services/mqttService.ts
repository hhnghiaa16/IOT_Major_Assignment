/**
 * MQTT Service
 * Handles all MQTT-related API calls (device commands)
 */

import apiClient from './apiClient';
import type { DeviceCommandRequest, DeviceCommandResponse } from '../types';

export const mqttService = {
  /**
   * Send device command via MQTT
   * @returns Response with message confirming the value sent
   */
  sendDeviceCommand: async (command: DeviceCommandRequest): Promise<DeviceCommandResponse> => {
    const response = await apiClient.post<DeviceCommandResponse>(
      '/mqtt/device-command',
      command
    );

    // API trả về dạng { "message": "0.0", "topic": "..." }
    if (response.message === undefined) {
      throw new Error('Failed to send device command - no response message');
    }

    return response;
  },
};

export default mqttService;

