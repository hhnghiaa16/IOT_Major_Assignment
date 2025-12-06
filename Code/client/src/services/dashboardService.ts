/**
 * Dashboard Service
 * Handles all dashboard-related API calls
 */

import apiClient from './apiClient';
import type {
  DashboardBlock,
  DashboardBlocksResponse,
  CreateBlockRequest,
  CreateBlockResponse,
} from '../types';

export const dashboardService = {
  /**
   * Get dashboard blocks by type
   * @param typeBlock 0 for button blocks, 1 for chart blocks
   */
  getBlocks: async (typeBlock: number): Promise<DashboardBlock[]> => {
    const response = await apiClient.get<DashboardBlocksResponse>(
      `/dashborad/blocks?typeblock=${typeBlock}`
    );
    
    if (!response.success) {
      throw new Error(response.message || 'Failed to fetch blocks');
    }
    
    return response.blocks || [];
  },

  /**
   * Create or update a dashboard block
   */
  saveBlock: async (blockData: CreateBlockRequest): Promise<void> => {
    const response = await apiClient.post<CreateBlockResponse>(
      '/dashborad/block',
      blockData
    );
    
    if (!response.success) {
      throw new Error(response.message || 'Failed to save block');
    }
  },

  /**
   * Delete a dashboard block
   */
  deleteBlock: async (blockId: number): Promise<void> => {
    const response = await apiClient.delete<CreateBlockResponse>(
      `/dashborad/block?id=${blockId}`
    );
    
    if (!response.success) {
      throw new Error(response.message || 'Failed to delete block');
    }
  },
};

export default dashboardService;

