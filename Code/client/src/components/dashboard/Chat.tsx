import React, { useState, useEffect, useRef } from 'react';
import '../../styles/Chat.css';

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

const Chat: React.FC = () => {
    const [isOpen, setIsOpen] = useState(false);
    const [messages, setMessages] = useState<Message[]>([]);
    const [inputValue, setInputValue] = useState('');
    const [isLoading, setIsLoading] = useState(false);
    const messagesEndRef = useRef<HTMLDivElement>(null);
    const [clientId, setClientId] = useState<string>('');

    const fetchHistory = async (id: string) => {
        try {
            const response = await fetch(`http://localhost:8000/ai/conversation/${id}`);
            if (response.ok) {
                const data = await response.json();
                if (data.history) {
                    // Filter out system messages if you don't want to show them
                    const history = data.history.filter((msg: Message) => msg.role !== 'system');
                    setMessages(history);
                }
            }
        } catch (error) {
            console.error('Failed to fetch history:', error);
        }
    };

    // Initialize client_id
    useEffect(() => {
        let storedClientId = localStorage.getItem('chat_client_id');
        if (!storedClientId) {
            storedClientId = 'user_' + Math.random().toString(36).substr(2, 9);
            localStorage.setItem('chat_client_id', storedClientId);
        }
        setClientId(storedClientId);
        fetchHistory(storedClientId);
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, []);

    // Auto-scroll to bottom
    useEffect(() => {
        messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
    }, [messages, isOpen]);

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
        <div className="chat-widget">
            {!isOpen && (
                <button className="chat-toggle-btn" onClick={() => setIsOpen(true)}>
                    <svg viewBox="0 0 24 24" fill="currentColor">
                        <path d="M12 2C6.477 2 2 6.477 2 12c0 1.821.487 3.53 1.338 5.025l-1.034 3.774a1 1 0 001.235 1.235l3.774-1.034A9.96 9.96 0 0012 22c5.523 0 10-4.477 10-10S17.523 2 12 2zm0 18c-1.476 0-2.886-.38-4.137-1.048l-2.616.717.717-2.616A7.96 7.96 0 014 12c0-4.411 3.589-8 8-8s8 3.589 8 8-3.589 8-8 8z" />
                        <path d="M8.5 12a1.5 1.5 0 113 0 1.5 1.5 0 01-3 0zm5 0a1.5 1.5 0 113 0 1.5 1.5 0 01-3 0z" />
                    </svg>
                </button>
            )}

            {isOpen && (
                <div className="chat-window">
                    <div className="chat-header">
                        <div className="chat-header-info">
                            <div className="chat-avatar">
                                👩‍💻
                                <div className="chat-status-dot"></div>
                            </div>
                            <div className="chat-title">
                                <h3>AI Support</h3>
                                <p>Đang hoạt động</p>
                            </div>
                        </div>
                        <button className="chat-close-btn" onClick={() => setIsOpen(false)}>
                            <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                                <path d="M6 18L18 6M6 6l12 12" />
                            </svg>
                        </button>
                    </div>

                    <div className="chat-messages">
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

                    <div className="chat-input-area">
                        <input
                            type="text"
                            className="chat-input"
                            placeholder="Nhập tin nhắn..."
                            value={inputValue}
                            onChange={(e) => setInputValue(e.target.value)}
                            onKeyPress={handleKeyPress}
                            disabled={isLoading}
                        />
                        <button
                            className="chat-send-btn"
                            onClick={handleSendMessage}
                            disabled={!inputValue.trim() || isLoading}
                        >
                            <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
                                <path d="M2.01 21L23 12 2.01 3 2 10l15 2-15 2z" />
                            </svg>
                        </button>
                    </div>
                </div>
            )}
        </div>
    );
};

export default Chat;