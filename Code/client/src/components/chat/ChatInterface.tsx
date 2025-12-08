import React, { useState, useEffect, useRef } from 'react';
import deviceService from '../../services/deviceService';
import apiClient from '../../services/apiClient';

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

interface ConversationHistoryResponse {
    client_id: string;
    history: Message[];
    message_count: number;
}

interface ChatInterfaceProps {
    aiName: string;
    aiStyle: string;
}

const ChatInterface: React.FC<ChatInterfaceProps> = ({ aiName, aiStyle }) => {
    const [messages, setMessages] = useState<Message[]>([]);
    const [inputValue, setInputValue] = useState('');
    const [isLoading, setIsLoading] = useState(false);
    const messagesEndRef = useRef<HTMLDivElement>(null);
    const [clientId, setClientId] = useState<string>('');

    const fetchHistory = async (tokenVerify: string) => {
        try {
            const response = await apiClient.get<ConversationHistoryResponse>(
                `/ai/conversation?client_id=${tokenVerify}`
            );
            if (response.history) {
                const history = response.history.filter((msg: Message) => msg.role !== 'system');
                setMessages(history);
            }
        } catch (error) {
            console.error('Failed to fetch history:', error);
        }
    };

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
                setMessages(prev => prev.slice(0, -1));
                setInputValue(userMessage.content);
            }
        } catch (error) {
            console.error('Error sending message:', error);
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

    // Initialize client_id with master device token and load history
    useEffect(() => {
        const initializeChat = async () => {
            const tokenVerify = await getMasterDeviceToken();
            if (tokenVerify) {
                setClientId(tokenVerify);
                await fetchHistory(tokenVerify);
            } else {
                console.warn('Không tìm thấy thiết bị Master');
            }
        };
        initializeChat();
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, []);

    // Auto-scroll to bottom
    useEffect(() => {
        messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
    }, [messages]);

    return (
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
    );
};

export default ChatInterface;
