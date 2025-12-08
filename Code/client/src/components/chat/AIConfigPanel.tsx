import React, { useState, useEffect } from 'react';
import apiClient from '../../services/apiClient';
import deviceService from '../../services/deviceService';


interface AIConfig {
    id: number;
    created_at: string;
    style: string;
    describe: string;
    token_verify: string;
    name: string;
    user_id?: number;
    charactor_voice?: string;
    speed?: number;
}

interface AIConfigResponse {
    success: boolean;
    message: string;
    data: AIConfig[];
}

interface AIConfigPanelProps {
    onConfigChange?: (name: string, style: string) => void;
}

const AIConfigPanel: React.FC<AIConfigPanelProps> = ({ onConfigChange }) => {
    const [aiName, setAiName] = useState('AI Support');
    const [aiStyle, setAiStyle] = useState('Thân thiện, nhiệt tình');
    const [systemPrompt, setSystemPrompt] = useState('Bạn là một trợ lý AI hữu ích.');
    const [characterVoice, setCharacterVoice] = useState('myan');
    const [speed, setSpeed] = useState(1);
    const [isSavingConfig, setIsSavingConfig] = useState(false);
    const [masterTokenVerify, setMasterTokenVerify] = useState<string | null>(null);
    const [voiceList, setVoiceList] = useState<Record<string, string>>({});

    const getMasterDeviceToken = async (): Promise<string | null> => {
        try {
            const masterDevices = await deviceService.getMasterDevices();
            if (masterDevices && masterDevices.length > 0) {
                return masterDevices[0].token_verify || null;
            }
            return null;
        } catch (error) {
            console.error('Failed to get Master device:', error);
            return null;
        }
    };

    const fetchVoiceList = async () => {
        try {
            const response = await apiClient.get<{ success: boolean; data: Record<string, string> }>('/ai/get_list_voice');
            if (response.success && response.data) {
                setVoiceList(response.data);
            }
        } catch (error) {
            console.error('Failed to fetch voice list:', error);
        }
    };

    const fetchAIConfig = async () => {
        try {
            const response = await apiClient.get<AIConfigResponse>('/ai/config_ai');
            if (response.success && response.data && response.data.length > 0) {
                // Lấy config mới nhất (đầu tiên trong mảng)
                const latestConfig = response.data[0];
                const name = latestConfig.name || 'AI Support';
                const style = latestConfig.style || 'Thân thiện, nhiệt tình';
                const prompt = latestConfig.describe || 'Bạn là một trợ lý AI hữu ích.';
                const voice = latestConfig.charactor_voice || 'myan';
                const voiceSpeed = latestConfig.speed ?? 1;

                setAiName(name);
                setAiStyle(style);
                setSystemPrompt(prompt);
                setCharacterVoice(voice);
                setSpeed(voiceSpeed);

                // Notify parent component
                if (onConfigChange) {
                    onConfigChange(name, style);
                }
            }
        } catch (error) {
            console.error('Failed to fetch AI config:', error);
        }
    };

    const saveAIConfig = async () => {
        if (!masterTokenVerify) {
            alert('Không tìm thấy thiết bị Master. Vui lòng đăng ký thiết bị Master trước.');
            return;
        }

        setIsSavingConfig(true);
        try {
            const response = await apiClient.post<{ success: boolean; message: string; data: AIConfig[] }>(
                '/ai/config_ai',
                {
                    token_verify: masterTokenVerify,
                    name: aiName,
                    style: aiStyle,
                    describe: systemPrompt,
                    charactor_voice: characterVoice,
                    speed: speed
                }
            );

            if (response.success && response.data && response.data.length > 0) {
                alert('Lưu cấu hình AI thành công!');
                // Notify parent component
                if (onConfigChange) {
                    onConfigChange(aiName, aiStyle);
                }
            } else {
                alert(response.message || 'Lưu cấu hình thất bại');
            }
        } catch (error) {
            console.error('Failed to save AI config:', error);
            alert('Lưu cấu hình thất bại. Vui lòng thử lại.');
        } finally {
            setIsSavingConfig(false);
        }
    };

    // Initialize config on mount
    useEffect(() => {
        const initializeConfig = async () => {
            // Fetch voice list first
            await fetchVoiceList();

            // Fetch AI config (uses JWT auth, no token needed)
            await fetchAIConfig();

            // Get master device token for saving config
            const tokenVerify = await getMasterDeviceToken();
            if (tokenVerify) {
                setMasterTokenVerify(tokenVerify);
            } else {
                console.warn('Không tìm thấy thiết bị Master');
            }
        };
        initializeConfig();
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, []);

    return (
        <div className="chat-sidebar">
            <div className="sidebar-header">
                <h2 className="sidebar-title">Cấu hình AI</h2>
            </div>

            <div className="config-group">
                <label className="config-label">Tên AI</label>
                <input
                    type="text"
                    className="config-input"
                    value={aiName}
                    onChange={(e) => setAiName(e.target.value)}
                    placeholder="Nhập tên AI..."
                />
            </div>

            <div className="config-group">
                <label className="config-label">Phong cách</label>
                <select
                    className="config-select"
                    value={aiStyle}
                    onChange={(e) => setAiStyle(e.target.value)}
                >
                    <option value="Thân thiện, nhiệt tình">Thân thiện, nhiệt tình</option>
                    <option value="Chuyên nghiệp, trang trọng">Chuyên nghiệp, trang trọng</option>
                    <option value="Hài hước, dí dỏm">Hài hước, dí dỏm</option>
                    <option value="Ngắn gọn, súc tích">Ngắn gọn, súc tích</option>
                </select>
            </div>

            <div className="config-group">
                <label className="config-label">System Prompt</label>
                <textarea
                    className="config-textarea"
                    value={systemPrompt}
                    onChange={(e) => setSystemPrompt(e.target.value)}
                    placeholder="Nhập hướng dẫn cho AI..."
                />
            </div>

            <div className="config-group">
                <label className="config-label">Giọng nói</label>
                <select
                    className="config-select"
                    value={characterVoice}
                    onChange={(e) => setCharacterVoice(e.target.value)}
                >
                    {Object.entries(voiceList).map(([label, value]) => (
                        <option key={value} value={value}>
                            {label}
                        </option>
                    ))}
                </select>
            </div>

            <div className="config-group">
                <label className="config-label">Tốc độ giọng nói ({speed}x)</label>
                <input
                    type="range"
                    className="config-range"
                    min="-3"
                    max="3"
                    step="0.1"
                    value={speed}
                    onChange={(e) => setSpeed(parseFloat(e.target.value))}
                />
                <div className="config-range-labels">
                    <span>Chậm (-3x)</span>
                    <span>Bình thường (1x)</span>
                    <span>Nhanh (3x)</span>
                </div>
            </div>

            <button
                className="save-config-btn"
                onClick={saveAIConfig}
                disabled={isSavingConfig}
            >
                <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                    <path d="M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z" />
                    <polyline points="17 21 17 13 7 13 7 21" />
                    <polyline points="7 3 7 8 15 8" />
                </svg>
                {isSavingConfig ? 'Đang lưu...' : 'Lưu cấu hình'}
            </button>
        </div>
    );
};

export default AIConfigPanel;
