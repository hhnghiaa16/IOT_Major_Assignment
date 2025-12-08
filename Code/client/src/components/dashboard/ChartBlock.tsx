/**
 * ChartBlock Component (Dumb Component)
 * Pure UI component for displaying sensor data charts
 */

import React, { useState } from 'react';
import { useSensorData } from '../../hooks/useSensorData';
import { formatTimestamp, formatDate, formatValue } from '../../utils/formatters';
import type { ChartBlockProps, SensorData } from '../../types';

const ChartBlock: React.FC<ChartBlockProps> = ({ block, onConfigure, onDelete }) => {
  const { data, loading, error } = useSensorData({
    tokenVerify: block.token_verify,
    virtualPin: block.virtual_pin,
    limit: 10,
    refreshInterval: 60000,
  });

  const [hoveredIndex, setHoveredIndex] = useState<number | null>(null);

  const getNumericValue = (d: SensorData): number => {
    if (d.value_numeric !== null && d.value_numeric !== undefined) {
      return d.value_numeric;
    }
    if (d.value_string) {
      const parsed = parseFloat(d.value_string);
      return isNaN(parsed) ? 0 : parsed;
    }
    return 0;
  };

  const getDisplayValue = (d: SensorData): string => {
    if (d.value_numeric !== null && d.value_numeric !== undefined) {
      return d.value_numeric.toFixed(1);
    }
    return d.value_string || 'N/A';
  };

  // Fixed Y-axis range from 18 to 35 degrees
  const scaleMin = 18;
  const scaleMax = 35;
  const scaleRange = scaleMax - scaleMin;

  const generatePath = (): string => {
    if (data.length === 0) return '';

    return data
      .map((d, i) => {
        const x = (i / (data.length - 1 || 1)) * 100;
        const numericValue = getNumericValue(d);
        const y = 100 - ((numericValue - scaleMin) / scaleRange) * 100;
        return `${i === 0 ? 'M' : 'L'} ${x},${y}`;
      })
      .join(' ');
  };

  const yAxisLabels = [
    { value: scaleMax, y: 0 },
    { value: scaleMin + scaleRange * 0.75, y: 25 },
    { value: scaleMin + scaleRange * 0.5, y: 50 },
    { value: scaleMin + scaleRange * 0.25, y: 75 },
    { value: scaleMin, y: 100 },
  ];

  const xAxisStep = Math.max(1, Math.floor(data.length / 5));
  const xAxisLabels = data.filter((_, i) => i % xAxisStep === 0 || i === data.length - 1);

  const currentValue = data.length > 0 ? getDisplayValue(data[data.length - 1]) : '0';

  return (
    <div className="chart-block">
      <div className="block-header">
        <span className="block-label">{block.label_block}</span>
        <div className="block-actions">
          <button className="block-action-btn" onClick={onConfigure} title="Cấu hình">
            <svg width="16" height="16" viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="2">
              <circle cx="8" cy="8" r="2" />
              <path d="M8 2v1.33M8 12.67V14M2 8h1.33M12.67 8H14M3.05 3.05l.94.94M11.01 11.01l.94.94M3.05 12.95l.94-.94M11.01 4.99l.94-.94" />
            </svg>
          </button>
          <button className="block-action-btn block-action-delete" onClick={onDelete} title="Xóa">
            <svg width="16" height="16" viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="2">
              <path d="M4 4h8v10a1 1 0 01-1 1H5a1 1 0 01-1-1V4zM6 2h4M2 4h12" />
            </svg>
          </button>
        </div>
      </div>

      <div className="block-info">
        <span className="block-device">{block.device_name}</span>
        <span className="block-pin">Pin {block.virtual_pin}</span>
      </div>

      {loading ? (
        <div className="chart-loading">
          <svg className="spinner" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="3">
            <path d="M12 2v4M12 18v4M4.93 4.93l2.83 2.83M16.24 16.24l2.83 2.83M2 12h4M18 12h4M4.93 19.07l2.83-2.83M16.24 7.76l2.83-2.83" />
          </svg>
          <p>Đang tải dữ liệu...</p>
        </div>
      ) : error ? (
        <div className="chart-error">
          <p>{error}</p>
        </div>
      ) : data.length === 0 ? (
        <div className="chart-empty">
          <p>Chưa có dữ liệu từ cảm biến</p>
        </div>
      ) : (
        <>
          <div className="chart-current-value">
            <span className="chart-value">{currentValue}</span>
            <span className="chart-unit">{block.pin_label}</span>
          </div>

          <div className="chart-container">
            <div className="chart-y-axis-labels">
              {yAxisLabels.map((label, i) => (
                <div key={i} className="chart-y-label">
                  {formatValue(label.value)}
                </div>
              ))}
            </div>

            <div className="chart-svg-wrapper">
              <svg viewBox="0 0 100 100" preserveAspectRatio="none" className="chart-svg">
                <defs>
                  <linearGradient id={`gradient-${block.id}`} x1="0%" y1="0%" x2="0%" y2="100%">
                    <stop offset="0%" stopColor="#22d3ee" stopOpacity="0.3" />
                    <stop offset="100%" stopColor="#22d3ee" stopOpacity="0.05" />
                  </linearGradient>
                  <filter id={`glow-${block.id}`}>
                    <feGaussianBlur stdDeviation="1.5" result="coloredBlur" />
                    <feMerge>
                      <feMergeNode in="coloredBlur" />
                      <feMergeNode in="SourceGraphic" />
                    </feMerge>
                  </filter>
                </defs>

                {[0, 25, 50, 75, 100].map((y, i) => (
                  <line
                    key={`h-${i}`}
                    x1="0"
                    y1={y}
                    x2="100"
                    y2={y}
                    stroke="rgba(148, 163, 184, 0.3)"
                    strokeWidth="0.3"
                    strokeDasharray={y === 0 || y === 100 ? '0' : '2,2'}
                    opacity={y === 0 || y === 100 ? '0.5' : '0.3'}
                  />
                ))}

                {xAxisLabels.map((d, i) => {
                  const index = data.indexOf(d);
                  const x = (index / (data.length - 1 || 1)) * 100;
                  return (
                    <line
                      key={`v-${i}`}
                      x1={x}
                      y1="0"
                      x2={x}
                      y2="100"
                      stroke="rgba(148, 163, 184, 0.2)"
                      strokeWidth="0.3"
                      strokeDasharray="2,2"
                      opacity="0.2"
                    />
                  );
                })}

                <path d={`${generatePath()} L 100,100 L 0,100 Z`} fill={`url(#gradient-${block.id})`} />

                <path
                  d={generatePath()}
                  fill="none"
                  stroke="#22d3ee"
                  strokeWidth="2.5"
                  strokeLinecap="round"
                  strokeLinejoin="round"
                  className="chart-line"
                  style={{ filter: 'drop-shadow(0 0 4px rgba(34, 211, 238, 0.4))' }}
                />

                {data.map((d, i) => {
                  const x = (i / (data.length - 1 || 1)) * 100;
                  const numericValue = getNumericValue(d);
                  const y = 100 - ((numericValue - scaleMin) / scaleRange) * 100;
                  const isHovered = hoveredIndex === i;
                  const displayValue = getDisplayValue(d);

                  return (
                    <g key={d.id}>
                      <circle
                        cx={x}
                        cy={y}
                        r={isHovered ? '2.5' : '1.8'}
                        fill={isHovered ? '#ffffff' : '#22d3ee'}
                        stroke="#22d3ee"
                        strokeWidth={isHovered ? '2' : '1.5'}
                        style={{ cursor: 'pointer', transition: 'all 0.2s ease-in-out' }}
                        filter={isHovered ? `url(#glow-${block.id})` : undefined}
                        onMouseEnter={() => setHoveredIndex(i)}
                        onMouseLeave={() => setHoveredIndex(null)}
                      />

                      {isHovered && (
                        <g>
                          <rect
                            x={x < 50 ? x + 4 : x - 30}
                            y={y - 14}
                            width="26"
                            height="12"
                            rx="2"
                            fill="rgba(15, 23, 42, 0.95)"
                            stroke="#22d3ee"
                            strokeWidth="0.5"
                            opacity="0.95"
                          />
                          <text
                            x={x < 50 ? x + 17 : x - 17}
                            y={y - 8.5}
                            textAnchor="middle"
                            fill="#22d3ee"
                            fontSize="3.5"
                            fontWeight="600"
                          >
                            {displayValue}
                          </text>
                          <text
                            x={x < 50 ? x + 17 : x - 17}
                            y={y - 4.5}
                            textAnchor="middle"
                            fill="#94a3b8"
                            fontSize="2.5"
                          >
                            {formatTimestamp(d.timestamp)}
                          </text>
                        </g>
                      )}
                    </g>
                  );
                })}
              </svg>
            </div>

            <div className="chart-x-axis-labels">
              {xAxisLabels.map((d, i) => (
                <div key={i} className="chart-x-label">
                  <span className="chart-x-time">{formatTimestamp(d.timestamp)}</span>
                  <span className="chart-x-date">{formatDate(d.timestamp)}</span>
                </div>
              ))}
            </div>
          </div>

          <div className="chart-time-info">
            <span>{data.length}/10 điểm dữ liệu</span>
            {data.length > 0 && (
              <span>{formatTimestamp(data[data.length - 1].timestamp)} (tự động cập nhật mỗi 20s)</span>
            )}
          </div>
        </>
      )}
    </div>
  );
};

export default ChartBlock;
