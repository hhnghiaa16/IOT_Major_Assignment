import React, { useState, useEffect, useRef } from 'react';
import '../styles/ChatPage.css';
import apiClient from '../services/apiClient';
import deviceService from '../services/deviceService';
import type { Device } from '../types';

interface Message {
    role: 'user' | 'assistant' | 'system';
    content: string;
    timestamp?: string;
}

interface ChatResponse {
    client_id: string;
    response: string;
    tool_calls: any[];
    message_count: number;
}

interface AIConfig {
    id: number;
    created_at: string;
    style: string;
    describe: string;
    token_verify: string;
    name: string;
    user_id?: number;
}

interface AIConfigResponse {
    success: boolean;
    message: string;
    data: AIConfig[];
}

const ChatPage: React.FC = () => {
    // Chat State
    const [messages, setMessages] = useState<Message[]>([]);
    const [inputValue, setInputValue] = useState('');
    const [isLoading, setIsLoading] = useState(false);
    const messagesEndRef = useRef<HTMLDivElement>(null);
    const [clientId, setClientId] = useState<string>('');

    // Config State
    const [aiName, setAiName] = useState('AI Support');
    const [aiStyle, setAiStyle] = useState('Thân thiện, nhiệt tình');
    const [systemPrompt, setSystemPrompt] = useState('Bạn là một trợ lý AI hữu ích.');
    const [isSavingConfig, setIsSavingConfig] = useState(false);
    const [masterTokenVerify, setMasterTokenVerify] = useState<string | null>(null);

    const fetchHistory = async (id: string) => {
        try {
            const response = await fetch(`http://localhost:8000/ai/conversation/${id}`);
            if (response.ok) {
                const data = await response.json();
                if (data.history) {
                    const history = data.history.filter((msg: Message) => msg.role !== 'system');
                    setMessages(history);
                }
            }
        } catch (error) {
            console.error('Failed to fetch history:', error);
        }
    };

    const getMasterDeviceToken = async (): Promise<string | null> => {
        try {
            const devices = await deviceService.getDevices();
            const masterDevice = devices.find((device: Device) => device.device_type === 'MASTER' && device.token_verify);
            return masterDevice?.token_verify || null;
        } catch (error) {
            console.error('Failed to get Master device:', error);
            return null;
        }
    };

    const fetchAIConfig = async (tokenVerify: string) => {
        try {
            const response = await apiClient.get<AIConfigResponse>(`/ai/config_ai?token_verify=${tokenVerify}`);
            if (response.success && response.data && response.data.length > 0) {
                // Lấy config mới nhất (đầu tiên trong mảng)
                const latestConfig = response.data[0];
                setAiName(latestConfig.name || 'AI Support');
                setAiStyle(latestConfig.style || 'Thân thiện, nhiệt tình');
                setSystemPrompt(latestConfig.describe || 'Bạn là một trợ lý AI hữu ích.');
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
            const response = await apiClient.post<AIConfig[]>(
                '/ai/config_ai',
                {
                    token_verify: masterTokenVerify,
                    name: aiName,
                    style: aiStyle,
                    describe: systemPrompt
                }
            );
            
            if (response && response.length > 0) {
                alert('Lưu cấu hình AI thành công!');
            } else {
                alert('Lưu cấu hình thất bại');
            }
        } catch (error) {
            console.error('Failed to save AI config:', error);
            alert('Lưu cấu hình thất bại. Vui lòng thử lại.');
        } finally {
            setIsSavingConfig(false);
        }
    };

    // Initialize client_id, get Master device token and load AI config
    useEffect(() => {
        let storedClientId = localStorage.getItem('chat_client_id');
        if (!storedClientId) {
            storedClientId = 'user_' + Math.random().toString(36).substr(2, 9);
            localStorage.setItem('chat_client_id', storedClientId);
        }
        setClientId(storedClientId);
        fetchHistory(storedClientId);
        
        // Get Master device token_verify and load AI config
        const initializeConfig = async () => {
            const tokenVerify = await getMasterDeviceToken();
            if (tokenVerify) {
                setMasterTokenVerify(tokenVerify);
                await fetchAIConfig(tokenVerify);
            } else {
                console.warn('Không tìm thấy thiết bị Master');
            }
        };
        initializeConfig();
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, []);

    // Auto-scroll to bottom
    useEffect(() => {
        messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
    }, [messages]);

    const handleSendMessage = async () => {
        if (!inputValue.trim()) return;

        const userMessage: Message = {
            role: 'user',
            content: inputValue,
            timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })
        };

        setMessages(prev => [...prev, userMessage]);
        setInputValue('');
        setIsLoading(true);

        try {
            const response = await fetch('http://localhost:8000/ai/chat', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({
                    client_id: clientId,
                    message: userMessage.content,
                    metadata: {
                        user_agent: navigator.userAgent
                    }
                }),
            });

            if (response.ok) {
                const data: ChatResponse = await response.json();
                const assistantMessage: Message = {
                    role: 'assistant',
                    content: data.response,
                    timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })
                };
                setMessages(prev => [...prev, assistantMessage]);
            } else {
                console.error('Failed to send message');
                // Restore user message on error
                setMessages(prev => prev.slice(0, -1));
                setInputValue(userMessage.content);
            }
        } catch (error) {
            console.error('Error sending message:', error);
            // Restore user message on error
            setMessages(prev => prev.slice(0, -1));
            setInputValue(userMessage.content);
        } finally {
            setIsLoading(false);
        }
    };

    const handleKeyPress = (e: React.KeyboardEvent) => {
        if (e.key === 'Enter' && !e.shiftKey) {
            e.preventDefault();
            handleSendMessage();
        }
    };

    return (
        <div className="chat-page">
            {/* Left Sidebar: AI Configuration */}
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

            {/* Right Content: Chat Interface */}
            <div className="chat-main">
                <div className="chat-main-header">
                    <div className="chat-main-avatar">
                        👩‍💻
                    </div>
                    <div className="chat-main-info">
                        <h2>{aiName}</h2>
                        <p>Đang hoạt động • {aiStyle}</p>
                    </div>
                </div>

                <div className="chat-main-messages">
                    {messages.map((msg, index) => (
                        <div key={index} className={`message ${msg.role}`}>
                            <div className="message-content">{msg.content}</div>
                            {msg.timestamp && <div className="message-time">{msg.timestamp}</div>}
                        </div>
                    ))}
                    {isLoading && (
                        <div className="typing-indicator">
                            <div className="typing-dot"></div>
                            <div className="typing-dot"></div>
                            <div className="typing-dot"></div>
                        </div>
                    )}
                    <div ref={messagesEndRef} />
                </div>

                <div className="chat-main-input-area">
                    <input
                        type="text"
                        className="chat-main-input"
                        placeholder="Nhập tin nhắn..."
                        value={inputValue}
                        onChange={(e) => setInputValue(e.target.value)}
                        onKeyPress={handleKeyPress}
                        disabled={isLoading}
                    />
                    <button
                        className="chat-main-send-btn"
                        onClick={handleSendMessage}
                        disabled={!inputValue.trim() || isLoading}
                    >
                        <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
                            <path d="M2.01 21L23 12 2.01 3 2 10l15 2-15 2z" />
                        </svg>
                    </button>
                </div>
            </div>
        </div >
    );
};

export default ChatPage;