/**
 * OTA Service
 * Handles all OTA-related API calls
 */

import apiClient from './apiClient';
import type { CheckOTAResponse, OTAInfo } from '../types';

export const otaService = {
  /**
   * Check OTA info for a device
   * @param clientId - Device token_verify
   * @param type - 0 for check, 1 for update
   */
  checkOTAInfo: async (clientId: string, type: number = 0): Promise<OTAInfo> => {
    const response = await apiClient.post<CheckOTAResponse>('/ota/check-info-ota', {
      client_id: clientId,
      type: type,
    });

    if (!response.success || !response.data) {
      throw new Error(response.message || 'Failed to fetch OTA info');
    }

    return response.data;
  },
};

export default otaService;

