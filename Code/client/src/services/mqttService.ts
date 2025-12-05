/**
 * MQTT Service
 * Handles all MQTT-related API calls (device commands)
 */

import apiClient from './apiClient';
import type { DeviceCommandRequest, DeviceCommandResponse } from '../types';

export const mqttService = {
  /**
   * Send device command via MQTT
   */
  sendDeviceCommand: async (command: DeviceCommandRequest): Promise<void> => {
    const response = await apiClient.post<DeviceCommandResponse>(
      '/mqtt/device-command',
      command
    );
    
    if (!response.success) {
      throw new Error(response.message || 'Failed to send device command');
    }
  },
};

export default mqttService;

