/**
 * useDashboard Hook
 * Manages dashboard blocks and operations
 */

import { useState, useEffect, useCallback } from 'react';
import dashboardService from '../services/dashboardService';
import mqttService from '../services/mqttService';
import type { DashboardBlock } from '../types';

interface UseDashboardReturn {
  buttonBlocks: DashboardBlock[];
  chartBlocks: DashboardBlock[];
  loading: boolean;
  error: string | null;
  togglingBlockId: number | null;
  loadAllBlocks: () => Promise<void>;
  toggleButton: (block: DashboardBlock) => Promise<void>;
  deleteBlock: (block: DashboardBlock) => Promise<void>;
}

export const useDashboard = (): UseDashboardReturn => {
  const [buttonBlocks, setButtonBlocks] = useState<DashboardBlock[]>([]);
  const [chartBlocks, setChartBlocks] = useState<DashboardBlock[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [togglingBlockId, setTogglingBlockId] = useState<number | null>(null);

  const normalizeValue = (value: unknown): number => {
    if (value === undefined || value === null) return 0;
    if (typeof value === 'string') {
      const num = parseInt(value, 10);
      return isNaN(num) ? 0 : num >= 1 ? 1 : 0;
    }
    return Number(value) >= 1 ? 1 : 0;
  };

  const initializeButtonsToOff = async (buttons: DashboardBlock[]): Promise<void> => {
    const initPromises = buttons.map(async (block) => {
      try {
        await mqttService.sendDeviceCommand({
          token_verify: block.token_verify,
          virtual_pin: block.virtual_pin,
          value: 0,
        });
      } catch (err) {
        console.error(`Error initializing block ${block.id}:`, err);
      }
    });

    await Promise.allSettled(initPromises);
  };

  const loadAllBlocks = useCallback(async () => {
    setLoading(true);
    setError(null);

    try {
      const [buttons, charts] = await Promise.all([
        dashboardService.getBlocks(0),
        dashboardService.getBlocks(1),
      ]);

      const buttonsWithDefaultValue = buttons.map((block) => ({
        ...block,
        value: 0,
      }));

      setButtonBlocks(buttonsWithDefaultValue);
      setChartBlocks(charts);

      if (buttons.length > 0) {
        await initializeButtonsToOff(buttons);
      }
    } catch (err) {
      const errorMessage = err instanceof Error 
        ? err.message 
        : 'Lỗi khi tải dữ liệu';
      setError(errorMessage);
    } finally {
      setLoading(false);
    }
  }, []);

  useEffect(() => {
    loadAllBlocks();
  }, [loadAllBlocks]);

  const toggleButton = async (block: DashboardBlock): Promise<void> => {
    if (togglingBlockId === block.id) return;

    const currentValue = normalizeValue(block.value);
    const newValue = currentValue === 1 ? 0 : 1;

    setTogglingBlockId(block.id);

    // Optimistically update UI
    setButtonBlocks((prev) =>
      prev.map((b) => (b.id === block.id ? { ...b, value: newValue } : b))
    );

    // Send command (don't wait for response)
    try {
      await mqttService.sendDeviceCommand({
        token_verify: block.token_verify,
        virtual_pin: block.virtual_pin,
        value: newValue,
      });
    } catch (err) {
      console.error('Error sending command:', err);
      // Revert on error
      setButtonBlocks((prev) =>
        prev.map((b) => (b.id === block.id ? { ...b, value: currentValue } : b))
      );
    } finally {
      setTogglingBlockId(null);
    }
  };

  const deleteBlock = async (block: DashboardBlock): Promise<void> => {
    if (!window.confirm(`Bạn có chắc chắn muốn xóa block "${block.label_block}"?`)) {
      return;
    }

    try {
      await dashboardService.deleteBlock(block.id);
      await loadAllBlocks();
      alert('Đã xóa block thành công');
    } catch (err) {
      const errorMessage = err instanceof Error 
        ? err.message 
        : 'Lỗi khi xóa block';
      alert(errorMessage);
    }
  };

  return {
    buttonBlocks,
    chartBlocks,
    loading,
    error,
    togglingBlockId,
    loadAllBlocks,
    toggleButton,
    deleteBlock,
  };
};

export default useDashboard;

