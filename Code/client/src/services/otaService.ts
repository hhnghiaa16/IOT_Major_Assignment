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

  /**
   * Upload new firmware
   * @param formData - FormData containing file and metadata
   */
  uploadFirmware: async (formData: FormData): Promise<{ success: boolean; message?: string }> => {
    const token = localStorage.getItem('access_token');
    const API_BASE = process.env.REACT_APP_API_URL || 'http://localhost:8000';

    const headers: HeadersInit = {};
    if (token) {
      headers['Authorization'] = `Bearer ${token}`;
    }
    // Note: Content-Type is not set here to let the browser set it with the boundary for FormData

    const response = await fetch(`${API_BASE}/ota/upload`, {
      method: 'POST',
      headers: headers,
      body: formData,
    });

    const data = await response.json();

    if (!response.ok) {
      throw new Error(data.detail || data.message || 'Failed to upload firmware');
    }

    return data;
  },
};

export default otaService;

